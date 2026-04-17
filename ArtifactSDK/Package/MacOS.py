import os
import shutil
import subprocess
from pathlib import Path
import plistlib

APP_NAME = "Artifact"
BINARY_PATH = Path("Binaries/Artifact")  # your compiled binary
OUTPUT_DIR = Path("Dist")
APP_PATH = OUTPUT_DIR / f"{APP_NAME}.app"

BUNDLE_ID = "com.artifact"
EXECUTABLE_NAME = "Artifact"

def run(cmd):
    print(">", " ".join(cmd))
    subprocess.check_call(cmd)

def create_structure():
    (APP_PATH / "Contents/MacOS").mkdir(parents=True, exist_ok=True)
    (APP_PATH / "Contents/Resources").mkdir(parents=True, exist_ok=True)

def copy_binary():
    dest = APP_PATH / "Contents/MacOS" / EXECUTABLE_NAME
    shutil.copy2(BINARY_PATH, dest)
    os.chmod(dest, 0o755)

def copy_content():
    content_src = Path("Content")
    content_dest = APP_PATH / "Contents/Resources/Content"
    if content_dest.exists():
        shutil.rmtree(content_dest)
    shutil.copytree(content_src, content_dest)

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
    copy_content()
    create_plist()
    sign_app()
    verify()

    print(f"\n✅ App created: {APP_PATH}")