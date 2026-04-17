from enum import Enum, auto

class TargetType(Enum):
    Debug = auto()
    Dev = auto()
    Dist = auto()

def get_cpp_target_macro(target: TargetType) -> str:
    if target == TargetType.Debug or target == "Debug":
        return "AE_TARGET_DEBUG"
    elif target == TargetType.Dev or target == "Dev":
        return "AE_TARGET_DEV"
    elif target == TargetType.Dist or target == "Dist":
        return "AE_TARGET_DIST"
    else:
        raise RuntimeError(f"Unsupported target: {target}")