"""`artifact setup` — check and install the development dependencies.

Which dependencies exist depends on the machine the SDK runs on: AppleClang and
LunarG's macOS SDK on a Mac, the MSVC build tools and the Windows SDK installer
on Windows, GCC plus distro packages on Linux. Everything installs from the
command line; none of the installers open their GUI.
"""

import sys

from colorama import Fore, Style

from SDK.Platforms import PlatformType, get_current_platform
from SetupTool.Dependency import SetupError
from SetupTool.PipTools import CMake, Ninja
from SetupTool.Process import step
from SetupTool.Toolchain import GccToolchain, MSVCToolchain, WindowingLibraries, XcodeToolchain
from SetupTool.VulkanSDK import LinuxVulkanSDK, MacOSVulkanSDK, Win64VulkanSDK


def get_dependencies(platform: PlatformType) -> list:
    if platform == PlatformType.MacOS:
        return [XcodeToolchain(), CMake(), Ninja(), MacOSVulkanSDK()]
    if platform == PlatformType.Win64:
        return [MSVCToolchain(), CMake(), Ninja(), Win64VulkanSDK()]
    return [GccToolchain(), CMake(), Ninja(), WindowingLibraries(), LinuxVulkanSDK()]


def _resolve(dependencies: list, requested: list) -> list:
    if any(name.lower() == "all" for name in requested):
        return list(dependencies)
    selected = []
    for name in requested:
        matches = [dependency for dependency in dependencies if dependency.matches(name)]
        if not matches:
            available = ", ".join(sorted({dependency.key for dependency in dependencies}))
            raise SetupError(f"Unknown dependency '{name}'. Available: {available}, all.")
        selected += [dependency for dependency in matches if dependency not in selected]
    return selected


def _status_line(satisfied: bool, name: str, detail: str, width: int = 0) -> str:
    symbol = f"{Fore.GREEN}✓{Style.RESET_ALL}" if satisfied else f"{Fore.RED}✗{Style.RESET_ALL}"
    if not detail:
        return f"{symbol} {name}"
    detail = f"{Style.DIM}{detail}{Style.RESET_ALL}" if satisfied else detail
    return f"{symbol} {name.ljust(width)}  {detail}"


def print_report(platform: PlatformType, results: list) -> bool:
    """Print the check results; returns whether every dependency is satisfied."""
    width = max(len(dependency.name) for dependency, _ in results)
    print(f"\n{Style.BRIGHT}Development dependencies for {platform.name}{Style.RESET_ALL}\n")
    for dependency, result in results:
        print(_status_line(result.satisfied, dependency.name, result.detail, width))

    missing = [dependency for dependency, result in results if not result.satisfied]
    if not missing:
        print(f"\n{Fore.GREEN}All development dependencies are installed.{Style.RESET_ALL}")
        return True

    keys = " ".join(dict.fromkeys(dependency.key for dependency in missing))
    print(f"\nRun `{Style.BRIGHT}artifact setup {keys}{Style.RESET_ALL}` to install the missing dependencies.")
    print(f"Or run `{Style.BRIGHT}artifact setup all{Style.RESET_ALL}` to install all dependencies.")
    return False


def check_dependencies(dependencies: list) -> list:
    return [(dependency, dependency.check()) for dependency in dependencies]


def install_dependencies(dependencies: list, requested: list, force: bool) -> bool:
    """Install the requested dependencies; returns whether all of them ended up satisfied.

    A dependency that fails to install does not abort the run — the remaining
    ones are still attempted, and the failures are summarised at the end.
    """
    selected = _resolve(dependencies, requested)
    outstanding = []
    for dependency in selected:
        result = dependency.check()
        if result.satisfied and not force:
            print(_status_line(True, dependency.name, f"already installed  {result.detail}".strip()))
            continue
        print()
        step(f"Installing {dependency.name}")
        try:
            dependency.install()
        except SetupError as error:
            print(f"{Fore.RED}✖ {dependency.name}: {error}{Style.RESET_ALL}")
            outstanding.append(dependency)
            continue
        result = dependency.check()
        if result.satisfied:
            print(_status_line(True, dependency.name, f"installed  {result.detail}".strip()))
        else:
            print(f"{Fore.YELLOW}! {dependency.name} is installed but not detected: "
                  f"{result.detail}{Style.RESET_ALL}")
            outstanding.append(dependency)

    if outstanding:
        names = ", ".join(dependency.name for dependency in outstanding)
        print(f"\n{Fore.RED}Not installed: {names}{Style.RESET_ALL}")
        return False
    return True


def run_setup(requested: list, force: bool = False):
    platform = get_current_platform()
    dependencies = get_dependencies(platform)

    if not requested:
        satisfied = print_report(platform, check_dependencies(dependencies))
        sys.exit(0 if satisfied else 1)

    if not install_dependencies(dependencies, requested, force):
        sys.exit(1)
