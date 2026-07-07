import os
import json
import shutil
import subprocess
from pathlib import Path
import plistlib

from SDK.Paths import get_engine_path
from BuildTool.Generate import get_content_mounts

# Non-cooked content copied raw into the bundle, by extension.
# Cooked assets are handled separately.
CONTENT_WHITELIST = [".glsl"]

APP_NAME = "Artifact"
BINARY_PATH = Path("Binaries/Artifact")  # your compiled binary
OUTPUT_DIR = Path("Dist")
APP_PATH = OUTPUT_DIR / f"{APP_NAME}.app"

BUNDLE_ID = "com.artifact"
EXECUTABLE_NAME = "Artifact"

# Root of the Vulkan SDK whose runtime pieces get bundled into the .app. LunarG's installer exports
# VULKAN_SDK; otherwise fall back to /usr/local, which is where this project links Vulkan from.
VULKAN_SDK = Path(os.environ.get("VULKAN_SDK", "/usr/local"))

def run(cmd):
    print(">", " ".join(cmd))
    subprocess.check_call(cmd)

def create_structure():
    (APP_PATH / "Contents/MacOS").mkdir(parents=True, exist_ok=True)
    (APP_PATH / "Contents/Resources").mkdir(parents=True, exist_ok=True)
    (APP_PATH / "Contents/Frameworks").mkdir(parents=True, exist_ok=True)

def copy_binary():
    dest = APP_PATH / "Contents/MacOS" / EXECUTABLE_NAME
    shutil.copy2(BINARY_PATH, dest)
    os.chmod(dest, 0o755)

def copy_content(project_path):
    # Packaged builds collapse every content mount into one resource dir
    content_dest = APP_PATH / "Contents/Resources/Content"
    if content_dest.exists():
        shutil.rmtree(content_dest)
    content_dest.mkdir(parents=True, exist_ok=True)

    for _key, mount_dir in get_content_mounts(get_engine_path(), project_path, "MacOS"):
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
    """Bundle the Vulkan runtime so the .app is self-contained on any Apple Silicon Mac.

    Layout produced:
      Contents/Frameworks/libvulkan.1.dylib        - the Vulkan loader (linked via @rpath)
      Contents/Frameworks/libMoltenVK.dylib        - the Metal-backed Vulkan driver
      Contents/Resources/vulkan/icd.d/MoltenVK_icd.json - ICD manifest pointing at the bundled driver
      Contents/Resources/glslangValidator          - runtime GLSL->SPIR-V compiler

    All of these are universal (x86_64 + arm64) and depend only on system libraries, so copying
    them plus rewiring the loader search paths is the entire dependency closure.
    """
    frameworks = APP_PATH / "Contents/Frameworks"
    resources = APP_PATH / "Contents/Resources"

    loader_src = VULKAN_SDK / "lib/libvulkan.1.dylib"
    moltenvk_src = VULKAN_SDK / "lib/libMoltenVK.dylib"
    glslang_src = VULKAN_SDK / "bin/glslangValidator"
    icd_src = VULKAN_SDK / "share/vulkan/icd.d/MoltenVK_icd.json"

    for required in (loader_src, moltenvk_src, glslang_src, icd_src):
        if not required.exists():
            raise FileNotFoundError(
                f"Required Vulkan SDK component not found: {required}\n"
                f"Set the VULKAN_SDK environment variable to your Vulkan SDK root."
            )

    # shutil.copy2 follows symlinks, so the bundled files are the real binaries (the SDK ships
    # libvulkan.1.dylib and glslangValidator as symlinks to versioned/renamed targets).
    shutil.copy2(loader_src, frameworks / "libvulkan.1.dylib", follow_symlinks=True)
    shutil.copy2(moltenvk_src, frameworks / "libMoltenVK.dylib", follow_symlinks=True)

    glslang_dest = resources / "glslangValidator"
    shutil.copy2(glslang_src, glslang_dest, follow_symlinks=True)
    os.chmod(glslang_dest, 0o755)

    # Rewrite the ICD manifest's library_path to point at the bundled driver, relative to the
    # manifest location (Contents/Resources/vulkan/icd.d -> Contents/Frameworks/libMoltenVK.dylib).
    icd_dest_dir = resources / "vulkan/icd.d"
    icd_dest_dir.mkdir(parents=True, exist_ok=True)
    with open(icd_src) as f:
        manifest = json.load(f)
    manifest["ICD"]["library_path"] = "../../../Frameworks/libMoltenVK.dylib"
    with open(icd_dest_dir / "MoltenVK_icd.json", "w") as f:
        json.dump(manifest, f, indent=4)

def fix_linkage():
    """Repoint the executable's rpath from /usr/local/lib to the bundled Frameworks directory.

    The binary links @rpath/libvulkan.1.dylib and @rpath/libMoltenVK.dylib; the build adds an
    absolute /usr/local/lib rpath that only exists on a developer machine. Swap it for one relative
    to the executable so the bundled dylibs are found on any machine.
    """
    exe = APP_PATH / "Contents/MacOS" / EXECUTABLE_NAME

    run(["install_name_tool", "-add_rpath", "@executable_path/../Frameworks", str(exe)])

    # Drop the developer-only absolute rpath if present; tolerate its absence.
    try:
        subprocess.check_call(
            ["install_name_tool", "-delete_rpath", "/usr/local/lib", str(exe)],
            stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        pass

    # Sanity check: nothing the executable loads should still resolve through /usr/local.
    otool = subprocess.run(["otool", "-l", str(exe)], capture_output=True, text=True).stdout
    if "/usr/local/lib" in otool:
        print("⚠️ Executable still references /usr/local/lib after relinking")

def create_plist():
    plist = {
        "CFBundleName": APP_NAME,
        "CFBundleDisplayName": APP_NAME,
        "CFBundleIdentifier": BUNDLE_ID,
        "CFBundleVersion": "1.0.0",
        "CFBundleShortVersionString": "1.0.0",
        "CFBundleExecutable": EXECUTABLE_NAME,
        "CFBundlePackageType": "APPL",
        "CFBundleIconFile": "app.icns",
        "LSMinimumSystemVersion": "10.13",
    }

    with open(APP_PATH / "Contents/Info.plist", "wb") as f:
        plistlib.dump(plist, f)

def sign_app():
    # Use ad-hoc signing (no Apple account required)
    run([
        "codesign",
        "--deep",
        "--force",
        "--sign", "-",
        str(APP_PATH)
    ])

def verify():
    run(["codesign", "--verify", "--deep", "--strict", str(APP_PATH)])

    try:
        run(["spctl", "--assess", "--verbose", str(APP_PATH)])
    except subprocess.CalledProcessError:
        print("⚠️ spctl rejected app (expected with ad-hoc signing)")

def make_icns(png_path: Path, output_icns: Path):
    iconset_dir = Path("icon.iconset")

    # clean old
    if iconset_dir.exists():
        shutil.rmtree(iconset_dir)

    iconset_dir.mkdir()

    # macOS expects multiple sizes
    sizes = [16, 32, 64, 128, 256, 512]

    for s in sizes:
        run([
            "sips",
            "-z", str(s), str(s),
            str(png_path),
            "--out", str(iconset_dir / f"icon_{s}x{s}.png")
        ])

        run([
            "cp",
            str(iconset_dir / f"icon_{s}x{s}.png"),
            str(iconset_dir / f"icon_{s}x{s}@2x.png")
        ])

    # convert to icns
    run([
        "iconutil",
        "-c", "icns",
        str(iconset_dir),
        "-o", str(output_icns)
    ])

    shutil.rmtree(iconset_dir)

def package_for_macos(project_path):
    if APP_PATH.exists():
        shutil.rmtree(APP_PATH)

    create_structure()
    make_icns(Path(f"{project_path}/Content/Icons/Icon.png"),  APP_PATH / "Contents/Resources/app.icns")

    copy_binary()
    copy_content(project_path)
    copy_dependencies()
    create_plist()
    fix_linkage()   # must run after binaries are in place and before signing (it invalidates sigs)
    sign_app()
    verify()

    print(f"\n✅ App created: {APP_PATH}")