"""The Vulkan SDK, installed head-less on every development platform.

macOS and Windows get LunarG's official SDK, driven through the command-line
interface of its Qt Installer Framework installer instead of the GUI. Linux uses
LunarG's apt repository where one exists for the distribution, and the distro's
own Vulkan development packages otherwise.
"""

import ctypes.util
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path

from SetupTool.Dependency import CheckResult, Dependency, SetupError
from SetupTool.Download import download_file, extract_zip, read_json, url_exists
from SetupTool.PackageManager import detect_package_manager, install_packages, read_os_release
from SetupTool.Process import command_output, note, run_command, sudo_prefix

LATEST_VERSIONS_URL = "https://vulkan.lunarg.com/sdk/latest.json"
MACOS_DOWNLOAD_URL = "https://sdk.lunarg.com/sdk/download/latest/mac/vulkan_sdk.zip"
WINDOWS_DOWNLOAD_URL = "https://sdk.lunarg.com/sdk/download/latest/windows/vulkan_sdk.exe"
LUNARG_SIGNING_KEY_URL = "https://packages.lunarg.com/lunarg-signing-key-pub.asc"
LUNARG_LIST_URL = "https://packages.lunarg.com/vulkan/lunarg-vulkan-{codename}.list"


def latest_sdk_version(platform_key: str) -> str:
    version = read_json(LATEST_VERSIONS_URL).get(platform_key)
    if not version:
        raise SetupError(f"LunarG lists no current SDK for '{platform_key}'.")
    return version


def unattended_installer_arguments(root: Path) -> list:
    """Arguments that drive LunarG's Qt installer without any GUI interaction."""
    return ["--root", str(root), "--accept-licenses", "--default-answer", "--confirm-command", "install"]


def version_from_path(path) -> str:
    match = re.search(r"(\d+(?:\.\d+)+)", str(path))
    return match.group(1) if match else "unknown version"


def display_path(path) -> str:
    text = str(path)
    home = str(Path.home())
    return text.replace(home, "~") if text.startswith(home) else text


def _newest(paths: list):
    def version_key(path):
        return tuple(int(part) for part in version_from_path(path).split(".") if part.isdigit())

    return max(paths, key=version_key) if paths else None


# --- macOS -----------------------------------------------------------------

# VulkanAPI links these by absolute path on macOS (see the module's
# ThirdParty/VulkanSDK/CMakeLists.txt), so an SDK that was unpacked but never
# installed system-wide is not enough to build.
MACOS_SYSTEM_LIBRARIES = [Path("/usr/local/lib/libvulkan.dylib"), Path("/usr/local/lib/libMoltenVK.dylib")]


def _is_macos_sdk(root: Path) -> bool:
    return (root / "include/vulkan/vulkan.h").exists()


def _macos_sdk_root():
    """The SDK's platform directory — what LunarG's setup-env.sh exports as VULKAN_SDK."""
    environment_root = os.environ.get("VULKAN_SDK")
    if environment_root and _is_macos_sdk(Path(environment_root)):
        return Path(environment_root)
    return _newest([root for root in (Path.home() / "VulkanSDK").glob("*/macOS") if _is_macos_sdk(root)])


def _find_macos_installer(extracted: Path) -> Path:
    for pattern in ("*.app", "*/*.app"):
        for application in sorted(extracted.glob(pattern)):
            executables = sorted(path for path in (application / "Contents/MacOS").glob("*")
                                 if path.is_file() and os.access(path, os.X_OK))
            if executables:
                return executables[0]
    raise SetupError(f"No Vulkan SDK installer application found in {extracted}")


class MacOSVulkanSDK(Dependency):
    key = "vulkan"
    name = "Vulkan SDK"
    aliases = ("vulkansdk", "vulkan-sdk", "moltenvk")

    def check(self) -> CheckResult:
        root = _macos_sdk_root()
        if root is None:
            return CheckResult.missing("no SDK in $VULKAN_SDK or ~/VulkanSDK")
        version = version_from_path(root)
        uninstalled = [library.name for library in MACOS_SYSTEM_LIBRARIES if not library.exists()]
        if uninstalled:
            return CheckResult.missing(f"{version} found, but {', '.join(uninstalled)} missing from /usr/local")
        return CheckResult.found(f"{version}  {display_path(root)}")

    def install(self):
        unpacked = _macos_sdk_root()
        if unpacked is not None and (unpacked.parent / "install_vulkan.py").exists() \
                and any(not library.exists() for library in MACOS_SYSTEM_LIBRARIES):
            # The SDK is there, only the /usr/local half of it is not — no point
            # in downloading a few hundred megabytes again for that.
            note(f"Vulkan SDK {version_from_path(unpacked)} is already unpacked in "
                 f"{display_path(unpacked.parent)}.")
            self._install_system_libraries(unpacked.parent)
            return

        version = latest_sdk_version("mac")
        root = Path.home() / "VulkanSDK" / version
        if root.exists():
            note(f"{display_path(root)} already exists — remove it (or run its MaintenanceTool) "
                 f"if the installer refuses to install into it.")
        with tempfile.TemporaryDirectory(prefix="artifact-vulkan-") as temporary_dir:
            archive = Path(temporary_dir) / "vulkan_sdk.zip"
            extracted = Path(temporary_dir) / "installer"
            download_file(MACOS_DOWNLOAD_URL, archive, f"Downloading Vulkan SDK {version}")
            extract_zip(archive, extracted, "Extracting the installer")
            installer = _find_macos_installer(extracted)
            note(f"Installing the Vulkan SDK into {display_path(root)}.")
            returncode = run_command([installer] + unattended_installer_arguments(root), allow_failure=True)
        if not (root / "macOS/lib").is_dir():
            raise SetupError(f"The Vulkan SDK installer did not populate {root} "
                             f"(exit code {returncode}).", returncode or 1)
        self._install_system_libraries(root)
        note(f"Add 'export VULKAN_SDK={display_path(root)}/macOS' to your shell profile "
             f"(or source {display_path(root)}/setup-env.sh) to point tooling at this SDK.")

    def _install_system_libraries(self, root: Path):
        """Copy the loader, layers and MoltenVK ICD into /usr/local.

        The GUI installer does this from a component script; head-less it is the
        SDK's own install_vulkan.py, which needs root to write /usr/local.
        """
        if all(library.exists() for library in MACOS_SYSTEM_LIBRARIES):
            return
        script = root / "install_vulkan.py"
        if not script.exists():
            raise SetupError(f"{script} is missing; cannot install the Vulkan loader into /usr/local.")
        note("Installing the Vulkan loader and MoltenVK into /usr/local — this needs administrator rights.")
        run_command(sudo_prefix() + [sys.executable, script,
                                     "--install-json-location", str(root), "--force-install"])


# --- Windows ---------------------------------------------------------------

def _is_windows_sdk(root: Path) -> bool:
    return (root / "Include/vulkan/vulkan.h").exists() and (root / "Lib/vulkan-1.lib").exists()


def _windows_sdk_root():
    environment_root = os.environ.get("VULKAN_SDK")
    if environment_root and _is_windows_sdk(Path(environment_root)):
        return Path(environment_root)
    install_dir = Path(os.environ.get("SystemDrive", "C:") + "\\VulkanSDK")
    return _newest([root for root in install_dir.glob("*") if root.is_dir() and _is_windows_sdk(root)])


class Win64VulkanSDK(Dependency):
    key = "vulkan"
    name = "Vulkan SDK"
    aliases = ("vulkansdk", "vulkan-sdk")

    def check(self) -> CheckResult:
        root = _windows_sdk_root()
        if root is None:
            return CheckResult.missing("no SDK in %VULKAN_SDK% or C:\\VulkanSDK")
        return CheckResult.found(f"{version_from_path(root)}  {root}")

    def install(self):
        version = latest_sdk_version("windows")
        root = Path(os.environ.get("SystemDrive", "C:") + "\\VulkanSDK") / version
        if root.exists():
            note(f"{root} already exists — uninstall it (maintenancetool.exe) "
                 f"if the installer refuses to install into it.")
        with tempfile.TemporaryDirectory(prefix="artifact-vulkan-") as temporary_dir:
            installer = Path(temporary_dir) / "vulkan_sdk.exe"
            download_file(WINDOWS_DOWNLOAD_URL, installer, f"Downloading Vulkan SDK {version}")
            note(f"Installing the Vulkan SDK into {root}. Windows may ask for administrator rights.")
            returncode = run_command([installer] + unattended_installer_arguments(root), allow_failure=True)
        if not _is_windows_sdk(root):
            raise SetupError(f"The Vulkan SDK installer did not populate {root} "
                             f"(exit code {returncode}).", returncode or 1)
        self._set_environment_variable(root)

    def _set_environment_variable(self, root: Path):
        """Point VULKAN_SDK at the new SDK for this user.

        The installer only writes the machine-wide variable when it runs
        elevated, and packaging reads VULKAN_SDK to find the shader compiler.
        """
        if os.environ.get("VULKAN_SDK") == str(root):
            return
        run_command(["setx", "VULKAN_SDK", str(root)], allow_failure=True)
        note("VULKAN_SDK was set for your user account — open a new terminal for it to take effect.")


# --- Linux -----------------------------------------------------------------

VULKAN_PACKAGES = {
    "apt": ["libvulkan-dev", "vulkan-tools", "vulkan-validationlayers-dev", "spirv-tools", "glslang-tools"],
    "dnf": ["vulkan-headers", "vulkan-loader-devel", "vulkan-tools", "vulkan-validation-layers",
            "glslang", "spirv-tools"],
    "pacman": ["vulkan-headers", "vulkan-icd-loader", "vulkan-tools", "vulkan-validation-layers",
               "glslang", "spirv-tools"],
    "zypper": ["vulkan-devel", "vulkan-tools", "vulkan-validationlayers", "glslang-devel"],
}


def _linux_sdk_root():
    """A tarball SDK pointed at by VULKAN_SDK, if one is set up."""
    environment_root = os.environ.get("VULKAN_SDK")
    if environment_root and (Path(environment_root) / "include/vulkan/vulkan.h").exists():
        return Path(environment_root)
    return None


class LinuxVulkanSDK(Dependency):
    key = "vulkan"
    name = "Vulkan SDK"
    aliases = ("vulkansdk", "vulkan-sdk")

    def check(self) -> CheckResult:
        root = _linux_sdk_root()
        if root is not None:
            return CheckResult.found(f"{version_from_path(root)}  {display_path(root)}")
        missing = []
        if not any((Path(include) / "vulkan/vulkan.h").exists() for include in ("/usr/include", "/usr/local/include")):
            missing.append("vulkan headers")
        if ctypes.util.find_library("vulkan") is None:
            missing.append("libvulkan")
        if not shutil.which("glslangValidator") and not shutil.which("glslc"):
            missing.append("a shader compiler")
        if missing:
            return CheckResult.missing(f"{', '.join(missing)} not found")
        version = command_output(["pkg-config", "--modversion", "vulkan"]) or "system packages"
        return CheckResult.found(version)

    def install(self):
        if self._install_from_lunarg_repository():
            return
        install_packages(VULKAN_PACKAGES, "the Vulkan development packages")

    def _install_from_lunarg_repository(self) -> bool:
        """Install LunarG's `vulkan-sdk` package where a repository exists for this release."""
        manager = detect_package_manager()
        if manager is None or manager.name != "apt":
            return False
        os_release = read_os_release()
        codename = os_release.get("VERSION_CODENAME") or os_release.get("UBUNTU_CODENAME")
        if not codename:
            return False
        list_url = LUNARG_LIST_URL.format(codename=codename)
        if not url_exists(list_url):
            note(f"LunarG has no repository for '{codename}'; using the distribution's Vulkan packages.")
            return False
        with tempfile.TemporaryDirectory(prefix="artifact-vulkan-") as temporary_dir:
            signing_key = Path(temporary_dir) / "lunarg.asc"
            source_list = Path(temporary_dir) / f"lunarg-vulkan-{codename}.list"
            download_file(LUNARG_SIGNING_KEY_URL, signing_key, "Downloading the LunarG signing key")
            download_file(list_url, source_list, "Downloading the LunarG package list")
            note("Registering LunarG's package repository — this needs administrator rights.")
            run_command(sudo_prefix() + ["install", "-m", "0644", str(signing_key),
                                         "/etc/apt/trusted.gpg.d/lunarg.asc"])
            run_command(sudo_prefix() + ["install", "-m", "0644", str(source_list),
                                         f"/etc/apt/sources.list.d/lunarg-vulkan-{codename}.list"])
        manager.refresh()
        manager.install(["vulkan-sdk"])
        return True
