import os
import shutil
import subprocess
from pathlib import Path

from PIL import Image

from SDK.Job import JobError
from SDK.Util import smart_open
from SetupTool.AppImageTool import get_appimagetool, get_architecture

APP_NAME = "Artifact"
BINARY_PATH = Path("Binaries/Artifact")
OUTPUT_DIR = Path("Dist")
APPDIR_PATH = OUTPUT_DIR / f"{APP_NAME}.AppDir"

LIBRARY_DIR = "usr/lib"
ICON_SIZE = 256

SYSTEM_LIBRARIES = (
    "ld-linux", "libc.", "libm.", "libdl.", "libpthread.", "librt.", "libresolv.", "libutil.",
    "libnsl.", "libcrypt.", "libstdc++.", "libgcc_s.",
    "libGL", "libEGL", "libOpenGL", "libGLdispatch", "libglapi.", "libgbm.", "libdrm", "libvulkan.",
    "libX", "libxcb", "libxkbcommon", "libwayland-", "libxshmfence.",
    "libasound.", "libpulse", "libdbus-1.", "libudev.", "libsystemd.", "libselinux.",
)

DESKTOP_ENTRY = """[Desktop Entry]
Type=Application
Name={name}
Exec={name}
Icon={name}
Categories=Game;
Terminal=false
"""

APPRUN = """#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/{libraries}${{LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}}"
exec "$HERE/usr/bin/{name}" "$@"
"""


def create_appdir(appdir: Path, binary: Path, project: Path):
    (appdir / "usr/bin").mkdir(parents=True)
    (appdir / LIBRARY_DIR).mkdir(parents=True)

    executable = appdir / "usr/bin" / APP_NAME
    shutil.copy2(binary, executable)
    executable.chmod(0o755)

    # Packaged builds collapse every content mount into one directory next to the executable.
    # Everything shipped is cooked: assets plus the compiled ShaderLibrary.
    shutil.copytree(project / OUTPUT_DIR / "Cooked", appdir / "usr/bin/Content")

    write_desktop_entry(appdir)
    write_icon(appdir, project / "Content/Icons/Icon.png")
    write_apprun(appdir)


def write_desktop_entry(appdir: Path):
    entry = DESKTOP_ENTRY.format(name=APP_NAME)
    for path in (appdir / f"{APP_NAME}.desktop",
                 appdir / "usr/share/applications" / f"{APP_NAME}.desktop"):
        with smart_open(str(path)) as desktop:
            desktop.write(entry)


def write_icon(appdir: Path, icon_png: Path):
    if not icon_png.exists():
        raise JobError(f"No application icon at {icon_png}")

    installed = appdir / f"usr/share/icons/hicolor/{ICON_SIZE}x{ICON_SIZE}/apps/{APP_NAME}.png"
    installed.parent.mkdir(parents=True)
    Image.open(icon_png).convert("RGBA").resize((ICON_SIZE, ICON_SIZE), Image.LANCZOS).save(installed)

    # appimagetool resolves the desktop entry's Icon= against the AppDir root, and links .DirIcon to it.
    shutil.copy2(installed, appdir / f"{APP_NAME}.png")


def write_apprun(appdir: Path):
    apprun = appdir / "AppRun"
    with smart_open(str(apprun)) as script:
        script.write(APPRUN.format(name=APP_NAME, libraries=LIBRARY_DIR))
    apprun.chmod(0o755)


def library_dependencies(binary: Path):
    """The shared libraries ``binary`` resolves to on this machine, as (soname, path) pairs."""
    output = subprocess.run(["ldd", str(binary)], capture_output=True, text=True).stdout
    for line in output.splitlines():
        soname, separator, resolved = line.strip().partition(" => ")
        if not separator:
            continue
        path = resolved.split(" (")[0].strip()
        if path and os.path.exists(path):
            yield soname.strip(), Path(path)


def bundle_libraries(binary: Path, destination: Path):
    for soname, path in library_dependencies(binary):
        if soname.startswith(SYSTEM_LIBRARIES):
            continue
        shutil.copy2(path, destination / soname, follow_symlinks=True)


def build_appimage(appdir: Path, output: Path, args):
    tool = get_appimagetool()
    if tool is None:
        raise JobError("No appimagetool found. Run `artifact setup appimagetool` to install one.")

    command = [str(tool)]
    if args.sign or args.sign_key:
        if not shutil.which("gpg") and not shutil.which("gpg2"):
            raise JobError("Signing an AppImage needs GnuPG; install gpg or package without --sign.")
        command.append("--sign")
        if args.sign_key:
            command += ["--sign-key", args.sign_key]
    command += ["--no-appstream", str(appdir), str(output)]

    output.unlink(missing_ok=True)
    # Extract-and-run rather than mounting: appimagetool is itself an AppImage, and most
    # distributions no longer ship the libfuse2 its runtime would otherwise need.
    environment = dict(os.environ, ARCH=get_architecture(), APPIMAGE_EXTRACT_AND_RUN="1")
    if subprocess.run(command, env=environment).returncode != 0:
        raise JobError("appimagetool failed to build the AppImage.")


def package_for_linux(project_path: str, args):
    project = Path(project_path)
    binary = project / BINARY_PATH
    if not binary.exists():
        raise JobError(f"The Linux build produced no {binary}")

    appdir = project / APPDIR_PATH
    if appdir.exists():
        shutil.rmtree(appdir)

    create_appdir(appdir, binary, project)
    bundle_libraries(binary, appdir / LIBRARY_DIR)

    output = project / OUTPUT_DIR / f"{APP_NAME}-{get_architecture()}.AppImage"
    build_appimage(appdir, output, args)

    print(f"\n✔ AppImage created: {output}")
