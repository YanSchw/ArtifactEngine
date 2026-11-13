import os
from pathlib import Path

from SDK.Job import JobError
from SDK.Util import smart_open

DEPLOYMENT_TARGET = "17.0"
ARCHITECTURE = "arm64"


def sdk_name(simulator: bool) -> str:
    return "iphonesimulator" if simulator else "iphoneos"


def vulkan_sdk_root() -> Path:
    from SetupTool.VulkanSDK import ios_sdk_root
    root = ios_sdk_root()
    if root is None:
        raise JobError("No iOS Vulkan SDK found. Install the iOS component of the LunarG Vulkan SDK "
                       "(it ships MoltenVK.xcframework) next to the macOS one in ~/VulkanSDK.", 1)
    return root


def moltenvk_framework(simulator: bool) -> Path:
    slice_name = "ios-arm64_x86_64-simulator" if simulator else "ios-arm64"
    framework = vulkan_sdk_root() / "lib/MoltenVK.xcframework" / slice_name / "MoltenVK.framework"
    if not framework.exists():
        raise JobError(f"MoltenVK slice missing: {framework}", 1)
    return framework


def write_toolchain(project_path: str, simulator: bool) -> str:
    path = f"{project_path}/Build/Intermediate/Toolchains/iOS-{sdk_name(simulator)}.cmake"
    root = vulkan_sdk_root().as_posix()
    with smart_open(path) as f:
        f.write(f"""set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_SYSROOT {sdk_name(simulator)})
set(CMAKE_OSX_ARCHITECTURES {ARCHITECTURE})
set(CMAKE_OSX_DEPLOYMENT_TARGET {DEPLOYMENT_TARGET})
set(AE_IOS_SIMULATOR {"ON" if simulator else "OFF"})
set(AE_VULKAN_INCLUDE_DIR "{root}/include")
set(AE_MOLTENVK_FRAMEWORK "{moltenvk_framework(simulator).as_posix()}")
""")
    return path
