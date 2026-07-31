"""Distro package-manager detection for the Linux installers.

Linux dependencies are installed through whatever package manager the distro
ships, so every Linux dependency declares its package names per manager
(``{"apt": [...], "dnf": [...]}``) and hands them to ``install_packages``.
"""

import shutil

from SetupTool.Dependency import SetupError
from SetupTool.Process import note, run_command, sudo_prefix


class PackageManager:
    def __init__(self, name: str, command: str, install_args: list, refresh_args: list = None):
        self.name = name
        self.command = command
        self.install_args = install_args
        self.refresh_args = refresh_args

    def refresh(self):
        if self.refresh_args:
            run_command(sudo_prefix() + [self.command] + self.refresh_args)

    def install(self, packages: list):
        run_command(sudo_prefix() + [self.command] + self.install_args + packages)


PACKAGE_MANAGERS = [
    PackageManager("apt", "apt-get", ["install", "-y"], ["update"]),
    PackageManager("dnf", "dnf", ["install", "-y"], None),
    PackageManager("pacman", "pacman", ["-S", "--needed", "--noconfirm"], ["-Sy"]),
    PackageManager("zypper", "zypper", ["install", "-y"], ["refresh"]),
]


def detect_package_manager():
    for manager in PACKAGE_MANAGERS:
        if shutil.which(manager.command):
            return manager
    return None


def read_os_release() -> dict:
    values = {}
    try:
        with open("/etc/os-release") as os_release:
            for line in os_release:
                key, separator, value = line.strip().partition("=")
                if separator:
                    values[key] = value.strip('"')
    except OSError:
        pass
    return values


def install_packages(packages_by_manager: dict, description: str):
    """Install the packages that provide ``description`` on this distro."""
    manager = detect_package_manager()
    if manager is None:
        raise SetupError(f"No supported package manager found; install {description} manually.")
    packages = packages_by_manager.get(manager.name)
    if not packages:
        raise SetupError(f"No package list for '{manager.name}'; install {description} manually.")
    note(f"Installing {description} with {manager.name}. This needs administrator rights.")
    manager.refresh()
    manager.install(packages)
