import sys
from enum import Enum, auto

class PlatformType(Enum):
    Win64 = auto()
    MacOS = auto()
    Linux = auto()

def get_current_platform() -> PlatformType:
    if sys.platform.startswith("win"):
        return PlatformType.Win64
    elif sys.platform.startswith("darwin"):
        return PlatformType.MacOS
    elif sys.platform.startswith("linux"):
        return PlatformType.Linux
    else:
        raise RuntimeError(f"Unsupported development platform: {sys.platform}")
    
def get_cpp_platform_macro(platform: PlatformType) -> str:
    if platform == PlatformType.Win64:
        return "AE_PLATFORM_WIN64"
    elif platform == PlatformType.MacOS:
        return "AE_PLATFORM_MACOS"
    elif platform == PlatformType.Linux:
        return "AE_PLATFORM_LINUX"
    else:
        raise RuntimeError(f"Unsupported platform: {platform}")