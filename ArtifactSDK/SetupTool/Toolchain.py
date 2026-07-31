"""The host C++ toolchain: AppleClang on macOS, MSVC on Windows, GCC on Linux."""

import json
import os
import re
import shutil
import tempfile
from pathlib import Path

from SetupTool.Dependency import CheckResult, Dependency
from SetupTool.Download import download_file
from SetupTool.PackageManager import install_packages
from SetupTool.Process import (command_output, command_succeeds, first_line, note,
                               run_command, sudo_prefix)

# --- macOS -----------------------------------------------------------------

# softwareupdate only offers the Command Line Tools while this flag file exists;
# it is the same handshake the "Install" dialog uses, and it is what makes a
# headless install possible.
COMMAND_LINE_TOOLS_FLAG = Path("/tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress")


def _newest_command_line_tools_label():
    listing = command_output(["softwareupdate", "--list"], combined=True) or ""
    labels = re.findall(r"^\s*\*\s*Label:\s*(Command Line Tools.*)$", listing, re.MULTILINE)
    if not labels:
        return None

    def version_key(label: str):
        match = re.search(r"(\d+(?:\.\d+)*)\s*$", label.strip())
        return tuple(int(part) for part in match.group(1).split(".")) if match else (0,)

    return max((label.strip() for label in labels), key=version_key)


class XcodeToolchain(Dependency):
    key = "toolchain"
    name = "Xcode Command Line Tools"
    aliases = ("xcode", "clang", "appleclang")

    def check(self) -> CheckResult:
        developer_dir = command_output(["xcode-select", "--print-path"])
        if not developer_dir or not os.path.isdir(developer_dir):
            return CheckResult.missing("no active developer directory")
        clang = command_output(["xcrun", "--find", "clang"])
        if not clang:
            return CheckResult.missing("clang not found in the active developer directory")
        return CheckResult.found(first_line(command_output([clang, "--version"])) or developer_dir)

    def install(self):
        created_flag = not COMMAND_LINE_TOOLS_FLAG.exists()
        if created_flag:
            COMMAND_LINE_TOOLS_FLAG.touch()
        try:
            label = _newest_command_line_tools_label()
            if label is None:
                # Nothing on offer: either they are already installed and only
                # xcode-select points nowhere, or softwareupdate has no catalog.
                run_command(["xcode-select", "--install"], allow_failure=True)
                note("Complete the Command Line Tools installation in the dialog, then re-run `artifact setup`.")
                return
            note("Installing the Command Line Tools needs administrator rights.")
            run_command(sudo_prefix() + ["softwareupdate", "--install", label, "--verbose"])
        finally:
            if created_flag:
                COMMAND_LINE_TOOLS_FLAG.unlink(missing_ok=True)


# --- Windows ---------------------------------------------------------------

VSWHERE_RELATIVE_PATH = r"Microsoft Visual Studio\Installer\vswhere.exe"
VC_TOOLS_COMPONENT = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
VC_TOOLS_WORKLOAD = "Microsoft.VisualStudio.Workload.VCTools"
BUILD_TOOLS_BOOTSTRAPPER_URL = "https://aka.ms/vs/17/release/vs_BuildTools.exe"
BUILD_TOOLS_WINGET_ID = "Microsoft.VisualStudio.2022.BuildTools"


def find_vswhere():
    program_files = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = os.path.join(program_files, VSWHERE_RELATIVE_PATH)
    return vswhere if os.path.exists(vswhere) else None


def find_visual_studio():
    """The newest Visual Studio installation that carries the C++ toolset."""
    vswhere = find_vswhere()
    if not vswhere:
        return None
    output = command_output([vswhere, "-latest", "-products", "*",
                             "-requires", VC_TOOLS_COMPONENT, "-format", "json"])
    if not output:
        return None
    try:
        installations = json.loads(output.lstrip("\ufeff"))
    except ValueError:
        return None
    return installations[0] if installations else None


def find_vcvarsall():
    """Path to vcvarsall.bat of the newest C++-capable Visual Studio install."""
    installation = find_visual_studio()
    if not installation:
        return None
    vcvars = os.path.join(installation["installationPath"], "VC", "Auxiliary", "Build", "vcvarsall.bat")
    return vcvars if os.path.exists(vcvars) else None


class MSVCToolchain(Dependency):
    key = "toolchain"
    name = "Visual Studio C++ build tools"
    aliases = ("msvc", "vs", "buildtools", "vstoolchain")

    def check(self) -> CheckResult:
        installation = find_visual_studio()
        if installation:
            description = installation.get("displayName") or "Visual Studio"
            version = installation.get("installationVersion") or ""
            if find_vcvarsall() is None:
                return CheckResult.missing(f"{description} has no vcvarsall.bat")
            return CheckResult.found(f"{description} {version}".strip())
        if shutil.which("cl"):
            return CheckResult.found("cl.exe on PATH")
        return CheckResult.missing("no Visual Studio with the C++ toolset")

    def install(self):
        note("Installing the Visual Studio 2022 C++ build tools. Windows will ask for administrator rights.")
        if shutil.which("winget"):
            returncode = run_command(
                ["winget", "install", "--id", BUILD_TOOLS_WINGET_ID, "--exact",
                 "--accept-package-agreements", "--accept-source-agreements",
                 "--override", f"--quiet --wait --norestart --nocache "
                               f"--add {VC_TOOLS_WORKLOAD} --includeRecommended"],
                allow_failure=True)
            if returncode == 0:
                return
            note("winget could not install the build tools; falling back to the Visual Studio bootstrapper.")
        with tempfile.TemporaryDirectory(prefix="artifact-buildtools-") as temporary_dir:
            bootstrapper = Path(temporary_dir) / "vs_BuildTools.exe"
            download_file(BUILD_TOOLS_BOOTSTRAPPER_URL, bootstrapper, "Downloading Visual Studio Build Tools")
            # 3010 is the installer's "succeeded, reboot pending".
            run_command([bootstrapper, "--quiet", "--wait", "--norestart", "--nocache",
                         "--add", VC_TOOLS_WORKLOAD, "--includeRecommended"],
                        allowed_returncodes=(0, 3010))


# --- Linux -----------------------------------------------------------------

GCC_PACKAGES = {
    "apt": ["build-essential"],
    "dnf": ["gcc-c++", "make"],
    "pacman": ["base-devel"],
    "zypper": ["gcc-c++", "make"],
}


class GccToolchain(Dependency):
    key = "toolchain"
    name = "GCC toolchain"
    aliases = ("gcc", "g++", "build-essential")

    def check(self) -> CheckResult:
        compiler = shutil.which("g++") or shutil.which("clang++")
        if not compiler:
            return CheckResult.missing("no g++ or clang++ on PATH")
        return CheckResult.found(first_line(command_output([compiler, "--version"])) or compiler)

    def install(self):
        install_packages(GCC_PACKAGES, "the C++ toolchain")


# GLFW builds both an X11 and a Wayland backend on Linux, so both sets of
# development packages have to be present for the engine to configure.
WINDOWING_PACKAGES = {
    "apt": ["pkg-config", "libx11-dev", "libxrandr-dev", "libxinerama-dev", "libxcursor-dev",
            "libxi-dev", "libxkbcommon-dev", "libwayland-dev", "wayland-protocols"],
    "dnf": ["pkgconf-pkg-config", "libX11-devel", "libXrandr-devel", "libXinerama-devel",
            "libXcursor-devel", "libXi-devel", "libxkbcommon-devel", "wayland-devel",
            "wayland-protocols-devel"],
    "pacman": ["pkgconf", "libx11", "libxrandr", "libxinerama", "libxcursor", "libxi",
               "libxkbcommon", "wayland", "wayland-protocols"],
    "zypper": ["pkg-config", "libX11-devel", "libXrandr-devel", "libXinerama-devel",
               "libXcursor-devel", "libXi-devel", "libxkbcommon-devel", "wayland-devel",
               "wayland-protocols-devel"],
}

# The pkg-config modules GLFW's CMake requires for its X11 and Wayland backends.
WINDOWING_PKG_CONFIG_MODULES = ["x11", "xrandr", "xinerama", "xcursor", "xi", "xkbcommon",
                                "wayland-client", "wayland-cursor", "wayland-egl", "wayland-protocols"]


class WindowingLibraries(Dependency):
    key = "windowing"
    name = "Window system dev libraries"
    aliases = ("x11", "xorg", "wayland", "glfw")

    def check(self) -> CheckResult:
        if not shutil.which("pkg-config") and not shutil.which("pkgconf"):
            return CheckResult.missing("pkg-config not found")
        missing = [module for module in WINDOWING_PKG_CONFIG_MODULES
                   if not command_succeeds(["pkg-config", "--exists", module])]
        if missing:
            return CheckResult.missing(f"missing: {', '.join(missing)}")
        return CheckResult.found("X11 and Wayland development packages present")

    def install(self):
        install_packages(WINDOWING_PACKAGES, "the X11/Wayland development libraries")
