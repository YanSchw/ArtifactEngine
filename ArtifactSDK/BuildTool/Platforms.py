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