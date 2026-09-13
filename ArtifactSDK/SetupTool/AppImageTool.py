"""The tools that pack an AppDir into the .AppImage `artifact package` ships on Linux.

appimagetool itself is a single self-contained binary rather than a distribution package, so
it goes to a fixed place under the user's home. It refuses to run without
desktop-file-validate, which is a distribution package, so this installs both.
"""

import os
import platform
import shutil
from pathlib import Path

from SetupTool.Dependency import CheckResult, Dependency, SetupError
from SetupTool.Download import download_file, url_exists
from SetupTool.PackageManager import install_packages

APPIMAGETOOL_DIRECTORY = Path.home() / ".artifact" / "appimagetool"

# The tool moved out of AppImageKit into its own repository; the archived releases are still the
# fallback for architectures the new one has no continuous build for.
APPIMAGETOOL_URLS = (
    "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-{architecture}.AppImage",
    "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-{architecture}.AppImage",
)

DESKTOP_FILE_PACKAGES = {
    "apt": ["desktop-file-utils"],
    "dnf": ["desktop-file-utils"],
    "pacman": ["desktop-file-utils"],
    "zypper": ["desktop-file-utils"],
}

ARCHITECTURES = {
    "x86_64": "x86_64",
    "amd64": "x86_64",
    "aarch64": "aarch64",
    "arm64": "aarch64",
    "armv7l": "armhf",
    "i686": "i686",
}


def get_architecture() -> str:
    """The AppImage architecture name for this machine."""
    architecture = ARCHITECTURES.get(platform.machine().lower())
    if architecture is None:
        raise SetupError(f"No AppImage architecture for '{platform.machine()}'.")
    return architecture


def get_appimagetool():
    """The appimagetool to package with: one on PATH, else the one `artifact setup` downloaded."""
    installed = shutil.which("appimagetool")
    if installed:
        return Path(installed)
    downloaded = APPIMAGETOOL_DIRECTORY / f"appimagetool-{get_architecture()}.AppImage"
    return downloaded if os.access(downloaded, os.X_OK) else None


class AppImageTool(Dependency):
    key = "appimagetool"
    name = "AppImage packaging tools"
    aliases = ("appimage",)

    def check(self) -> CheckResult:
        tool = get_appimagetool()
        missing = []
        if tool is None:
            missing.append(f"appimagetool (looked on PATH and in {APPIMAGETOOL_DIRECTORY})")
        if not shutil.which("desktop-file-validate"):
            missing.append("desktop-file-validate")
        if missing:
            return CheckResult.missing(f"{', '.join(missing)} not found")
        return CheckResult.found(str(tool))

    def install(self):
        if not shutil.which("desktop-file-validate"):
            install_packages(DESKTOP_FILE_PACKAGES, "the desktop entry validator appimagetool requires")
        if shutil.which("appimagetool"):
            return

        architecture = get_architecture()
        destination = APPIMAGETOOL_DIRECTORY / f"appimagetool-{architecture}.AppImage"
        for url in APPIMAGETOOL_URLS:
            url = url.format(architecture=architecture)
            if not url_exists(url):
                continue
            download_file(url, destination, "Downloading appimagetool")
            destination.chmod(0o755)
            return
        raise SetupError(f"No appimagetool release for '{architecture}'.")
