import os
import shutil
from pathlib import Path

from SDK.Paths import get_engine_path
from BuildTool.Generate import get_content_mounts

# Non-cooked content copied raw into the package, by extension.
# Cooked assets are handled separately.
CONTENT_WHITELIST = [".glsl"]

APP_NAME = "Artifact"
BINARY_PATH = Path("Binaries/Artifact.exe")
OUTPUT_DIR = Path("Dist")
PACKAGE_DIR = OUTPUT_DIR / APP_NAME

EXECUTABLE_NAME = "Artifact.exe"

# Root of the Vulkan SDK whose runtime pieces get bundled alongside the executable. LunarG's
# installer exports VULKAN_SDK; there's no standard fallback location on Windows.
VULKAN_SDK = os.environ.get("VULKAN_SDK")

def create_structure():
    PACKAGE_DIR.mkdir(parents=True, exist_ok=True)

def copy_binary():
    shutil.copy2(BINARY_PATH, PACKAGE_DIR / EXECUTABLE_NAME)

def copy_content(project_path):
    # Packaged builds collapse every content mount into one directory next to the executable
    content_dest = PACKAGE_DIR / "Content"
    if content_dest.exists():
        shutil.rmtree(content_dest)
    content_dest.mkdir(parents=True, exist_ok=True)

    for _key, mount_dir in get_content_mounts(get_engine_path(), project_path, "Win64"):
        mount_path = Path(mount_dir)
        if not mount_path.is_dir():
            continue
        for src in mount_path.rglob("*"):
            if src.is_file() and src.suffix in CONTENT_WHITELIST:
                dest = content_dest / src.relative_to(mount_path)
                if dest.exists():
                    continue
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dest)

    # copy cooked assets into content_dest
    cooked_src = OUTPUT_DIR / "Cooked"
    shutil.copytree(cooked_src, content_dest, dirs_exist_ok=True)

def copy_dependencies():
    """Bundle the glslangValidator tool next to the executable.

    Unlike MacOS, Windows GPU drivers install the Vulkan loader (vulkan-1.dll) and register their
    own ICD system-wide, so neither needs to be bundled. The Vulkan SDK doesn't redistribute the
    loader either (Lib/vulkan-1.lib is import-only), it's purely a runtime-installed component.
    Only glslangValidator.exe is SDK-local and needs to travel with the package; Windows resolves
    it from the executable's own directory, so no linkage fix-up is needed.
    """
    if not VULKAN_SDK:
        raise RuntimeError("VULKAN_SDK environment variable is not set. Install the Vulkan SDK and set VULKAN_SDK to its root.")

    glslang_src = Path(VULKAN_SDK) / "Bin/glslangValidator.exe"
    if not glslang_src.exists():
        raise FileNotFoundError(f"Required Vulkan SDK component not found: {glslang_src}")

    shutil.copy2(glslang_src, PACKAGE_DIR / "glslangValidator.exe")

def package_for_win64(project_path):
    if PACKAGE_DIR.exists():
        shutil.rmtree(PACKAGE_DIR)

    create_structure()
    copy_binary()
    copy_content(project_path)
    copy_dependencies()

    print(f"\n✅ Package created: {PACKAGE_DIR}")
