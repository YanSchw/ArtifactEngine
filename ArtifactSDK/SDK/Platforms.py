import sys
from enum import Enum, auto

class PlatformType(Enum):
    Win64 = auto()
    MacOS = auto()
    Linux = auto()
    Web = auto()

def get_current_platform() -> PlatformType:
    if sys.platform.startswith("win"):
        return PlatformType.Win64
    elif sys.platform.startswith("darwin"):
        return PlatformType.MacOS
    elif sys.platform.startswith("linux"):
        return PlatformType.Linux
    else:
        raise RuntimeError(f"Unsupported development platform: {sys.platform}")

def get_platform(name) -> PlatformType:
    """Resolve a target name, however the user spelled it on the command line."""
    if isinstance(name, PlatformType):
        return name
    for platform in PlatformType:
        if platform.name.lower() == str(name).lower():
            return platform
    raise RuntimeError(f"Unsupported platform: {name}")

def get_platform_names() -> list[str]:
    return [platform.name for platform in PlatformType]

def get_cpp_platform_macro(platform) -> str:
    return {
        PlatformType.Win64: "AE_PLATFORM_WIN64",
        PlatformType.MacOS: "AE_PLATFORM_MACOS",
        PlatformType.Linux: "AE_PLATFORM_LINUX",
        PlatformType.Web: "AE_PLATFORM_WEB",
    }[get_platform(platform)]
