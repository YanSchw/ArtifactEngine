import shutil
from pathlib import Path

APP_NAME = "Artifact"
BINARY_PATH = Path("Binaries/Artifact.exe")
OUTPUT_DIR = Path("Dist")
PACKAGE_DIR = OUTPUT_DIR / APP_NAME

EXECUTABLE_NAME = "Artifact.exe"

def create_structure():
    PACKAGE_DIR.mkdir(parents=True, exist_ok=True)

def copy_binary():
    shutil.copy2(BINARY_PATH, PACKAGE_DIR / EXECUTABLE_NAME)

def copy_content(project_path):
    # Packaged builds collapse every content mount into one directory next to the executable.
    # Everything shipped is cooked: assets plus the compiled ShaderLibrary.
    content_dest = PACKAGE_DIR / "Content"
    if content_dest.exists():
        shutil.rmtree(content_dest)
    content_dest.mkdir(parents=True, exist_ok=True)

    cooked_src = OUTPUT_DIR / "Cooked"
    shutil.copytree(cooked_src, content_dest, dirs_exist_ok=True)

def package_for_win64(project_path):
    if PACKAGE_DIR.exists():
        shutil.rmtree(PACKAGE_DIR)

    create_structure()
    copy_binary()
    copy_content(project_path)

    print(f"\n✅ Package created: {PACKAGE_DIR}")
