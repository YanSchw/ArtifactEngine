import json
import plistlib
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

from SDK.Job import Job, JobError
from BuildTool.iOS import DEPLOYMENT_TARGET, moltenvk_framework, sdk_name
from Package.iOSProvisioning import Signing, ensure_signing

APP_NAME = "Artifact"
EXECUTABLE_NAME = "Artifact"
SIMULATOR_BUNDLE_ID = "com.artifact"
SCENE_DELEGATE_CLASS = "ArtifactSceneDelegate"

OUTPUT_DIR = Path("Dist")
APP_PATH = OUTPUT_DIR / f"{APP_NAME}.app"
BINARY_PATH = Path("Binaries") / f"{APP_NAME}.app" / EXECUTABLE_NAME


def run(cmd):
    print(">", " ".join(str(part) for part in cmd))
    subprocess.check_call([str(part) for part in cmd])


def report(message: str):
    if Job._active is not None:
        Job._active.persist(message)
    else:
        print(message)


def capture(cmd) -> str:
    return subprocess.run([str(part) for part in cmd], capture_output=True, text=True).stdout


def copy_binary():
    if not BINARY_PATH.exists():
        raise JobError(f"No iOS binary at {BINARY_PATH}", 1)
    destination = APP_PATH / EXECUTABLE_NAME
    shutil.copy2(BINARY_PATH, destination)
    destination.chmod(0o755)

    rpaths = re.findall(r"cmd LC_RPATH\s+cmdsize \d+\s+path (\S+)", capture(["otool", "-l", destination]))
    for rpath in rpaths:
        if not rpath.startswith("@"):
            run(["install_name_tool", "-delete_rpath", rpath, destination])


def copy_content():
    shutil.copytree(OUTPUT_DIR / "Cooked", APP_PATH / "Content")


def copy_frameworks(simulator: bool):
    frameworks = APP_PATH / "Frameworks"
    frameworks.mkdir()
    shutil.copytree(moltenvk_framework(simulator), frameworks / "MoltenVK.framework", symlinks=True)


def compile_icon(project_path: str, simulator: bool) -> dict:
    icon = Path(project_path) / "Content/Icons/Icon.png"
    if not icon.exists():
        return {}
    with tempfile.TemporaryDirectory(prefix="artifact-ios-icon-") as temporary:
        catalog = Path(temporary) / "Assets.xcassets"
        icon_set = catalog / "AppIcon.appiconset"
        icon_set.mkdir(parents=True)
        run(["sips", "-z", "1024", "1024", icon, "--out", icon_set / "Icon.png"])
        (catalog / "Contents.json").write_text(json.dumps({"info": {"author": "artifact", "version": 1}}))
        (icon_set / "Contents.json").write_text(json.dumps({
            "images": [{"filename": "Icon.png", "idiom": "universal", "platform": "ios", "size": "1024x1024"}],
            "info": {"author": "artifact", "version": 1},
        }))
        partial = Path(temporary) / "Partial.plist"
        run(["xcrun", "actool", catalog, "--compile", APP_PATH, "--platform", sdk_name(simulator),
             "--minimum-deployment-target", DEPLOYMENT_TARGET, "--app-icon", "AppIcon",
             "--target-device", "iphone", "--target-device", "ipad",
             "--output-partial-info-plist", partial, "--output-format", "human-readable-text"])
        return plistlib.loads(partial.read_bytes()) if partial.exists() else {}


def write_info_plist(bundle_id: str, simulator: bool, extra: dict):
    sdk = sdk_name(simulator)
    sdk_version = capture(["xcrun", "--sdk", sdk, "--show-sdk-version"]).strip()
    landscape = ["UIInterfaceOrientationLandscapeLeft", "UIInterfaceOrientationLandscapeRight"]
    plist = {
        "CFBundleDevelopmentRegion": "en",
        "CFBundleExecutable": EXECUTABLE_NAME,
        "CFBundleIdentifier": bundle_id,
        "CFBundleInfoDictionaryVersion": "6.0",
        "CFBundleName": APP_NAME,
        "CFBundleDisplayName": APP_NAME,
        "CFBundlePackageType": "APPL",
        "CFBundleShortVersionString": "1.0.0",
        "CFBundleVersion": "1",
        "CFBundleSupportedPlatforms": ["iPhoneSimulator" if simulator else "iPhoneOS"],
        "DTPlatformName": sdk,
        "DTPlatformVersion": sdk_version,
        "DTSDKName": f"{sdk}{sdk_version}",
        "MinimumOSVersion": DEPLOYMENT_TARGET,
        "LSRequiresIPhoneOS": True,
        "UIDeviceFamily": [1, 2],
        "UIRequiredDeviceCapabilities": ["arm64", "metal"],
        "UILaunchScreen": {},
        "UIRequiresFullScreen": True,
        "UIStatusBarHidden": True,
        "UISupportedInterfaceOrientations": landscape,
        "UISupportedInterfaceOrientations~ipad": landscape,
        "UIApplicationSceneManifest": {
            "UIApplicationSupportsMultipleScenes": False,
            "UISceneConfigurations": {
                "UIWindowSceneSessionRoleApplication": [{
                    "UISceneConfigurationName": "Default",
                    "UISceneDelegateClassName": SCENE_DELEGATE_CLASS,
                }],
            },
        },
        "ITSAppUsesNonExemptEncryption": False,
    }
    plist.update(extra)
    with open(APP_PATH / "Info.plist", "wb") as f:
        plistlib.dump(plist, f, fmt=plistlib.FMT_BINARY)


def sign(signing: Signing, bundle_id: str):
    identity = signing.identity if signing else "-"
    run(["codesign", "--force", "--sign", identity, "--timestamp=none", APP_PATH / "Frameworks/MoltenVK.framework"])
    if signing is None:
        run(["codesign", "--force", "--sign", "-", "--timestamp=none", APP_PATH])
        return

    shutil.copy2(signing.profile_path, APP_PATH / "embedded.mobileprovision")
    with tempfile.NamedTemporaryFile(suffix=".plist", delete=False) as entitlements:
        plistlib.dump(signing.entitlements(bundle_id), entitlements)
    try:
        run(["codesign", "--force", "--sign", identity, "--timestamp=none",
             "--entitlements", entitlements.name, "--generate-entitlement-der", APP_PATH])
    finally:
        Path(entitlements.name).unlink(missing_ok=True)
    run(["codesign", "--verify", "--deep", "--strict", APP_PATH])


def install(bundle_id: str, simulator: bool, device):
    if simulator:
        if "(Booted)" not in capture(["xcrun", "simctl", "list", "devices", "booted"]):
            raise JobError("No booted simulator. Start one (e.g. `open -a Simulator`) and run again.", 1)
        run(["xcrun", "simctl", "install", "booted", APP_PATH])
        run(["xcrun", "simctl", "launch", "booted", bundle_id])
        return
    if device is None:
        raise JobError("No iPhone connected. Plug it in, unlock it and trust this Mac, then run again.", 1)
    run(["xcrun", "devicectl", "device", "install", "app", "--device", device.identifier, APP_PATH])
    launch = ["xcrun", "devicectl", "device", "process", "launch", "--device", device.identifier, bundle_id]
    print(">", " ".join(launch))
    result = subprocess.run(launch, capture_output=True, text=True)
    output = result.stdout + result.stderr
    if result.returncode == 0:
        return
    if "explicitly trusted" in output or "Security" in output:
        report(f"⚠️ {APP_NAME} is installed on {device.name}, but iOS does not trust your developer certificate yet.\n"
              "   On the phone: Settings > General > VPN & Device Management > Developer App > "
              "Apple Development: <your Apple ID> > Trust, then open Artifact from the home screen.\n"
              "   This is needed once per certificate; later packages launch directly.")
    else:
        report(f"⚠️ {APP_NAME} is installed on {device.name} but could not be launched:\n{output.strip()}")


def prepare_package(project_path, args):
    args.ios_signing = None
    args.ios_device = None
    if getattr(args, "simulator", False):
        args.bundle_id = getattr(args, "bundle_id", None) or SIMULATOR_BUNDLE_ID
        return
    signing, bundle_id, device = ensure_signing(Path(project_path) / "Build/Intermediate/iOSProvisioning", args)
    args.ios_signing, args.bundle_id, args.ios_device = signing, bundle_id, device
    report(f"✔ Signing {bundle_id} with {signing.name}, profile '{signing.profile.get('Name', signing.profile_path.name)}'")


def package_for_ios(project_path, args):
    if not hasattr(args, "ios_signing"):
        prepare_package(project_path, args)
    simulator = getattr(args, "simulator", False)
    signing = args.ios_signing
    bundle_id = args.bundle_id

    if APP_PATH.exists():
        shutil.rmtree(APP_PATH)
    APP_PATH.mkdir(parents=True)

    copy_binary()
    copy_content()
    copy_frameworks(simulator)
    write_info_plist(bundle_id, simulator, compile_icon(project_path, simulator))
    sign(signing, bundle_id)

    report(f"✔ App created: {APP_PATH} ({bundle_id}, {'simulator' if simulator else 'device'})")
    if getattr(args, "install", False):
        install(bundle_id, simulator, args.ios_device)
