import hashlib
import json
import plistlib
import re
import shutil
import subprocess
import tempfile
from datetime import datetime, timezone
from pathlib import Path

from SDK.Job import JobError
from BuildTool.iOS import DEPLOYMENT_TARGET

PROFILE_DIRECTORIES = [
    Path.home() / "Library/Developer/Xcode/UserData/Provisioning Profiles",
    Path.home() / "Library/MobileDevice/Provisioning Profiles",
]
DEVELOPMENT_IDENTITY_PATTERN = re.compile(r'^\s*\d+\)\s+([0-9A-F]{40})\s+"((?:Apple Development|iPhone Developer)[^"]*)"')
STUB_NAME = "ArtifactProvisioning"

STUB_PROJECT = """// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 56;
	objects = {
		A10000000000000000000001 = {isa = PBXBuildFile; fileRef = A10000000000000000000002; };
		A10000000000000000000002 = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.c; path = main.c; sourceTree = "<group>"; };
		A10000000000000000000003 = {isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = @NAME@.app; sourceTree = BUILT_PRODUCTS_DIR; };
		A10000000000000000000004 = {isa = PBXGroup; children = (A10000000000000000000002, A10000000000000000000005, ); sourceTree = "<group>"; };
		A10000000000000000000005 = {isa = PBXGroup; children = (A10000000000000000000003, ); name = Products; sourceTree = "<group>"; };
		A10000000000000000000006 = {
			isa = PBXNativeTarget;
			buildConfigurationList = A10000000000000000000007;
			buildPhases = (A10000000000000000000008, );
			buildRules = ( );
			dependencies = ( );
			name = @NAME@;
			productName = @NAME@;
			productReference = A10000000000000000000003;
			productType = "com.apple.product-type.application";
		};
		A10000000000000000000008 = {isa = PBXSourcesBuildPhase; buildActionMask = 2147483647; files = (A10000000000000000000001, ); runOnlyForDeploymentPostprocessing = 0; };
		A10000000000000000000009 = {
			isa = PBXProject;
			attributes = {LastUpgradeCheck = 1500; TargetAttributes = {A10000000000000000000006 = {CreatedOnToolsVersion = 15.0; }; }; };
			buildConfigurationList = A1000000000000000000000A;
			compatibilityVersion = "Xcode 14.0";
			developmentRegion = en;
			hasScannedForEncodings = 0;
			knownRegions = (en, Base, );
			mainGroup = A10000000000000000000004;
			productRefGroup = A10000000000000000000005;
			projectDirPath = "";
			projectRoot = "";
			targets = (A10000000000000000000006, );
		};
		A1000000000000000000000B = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				IPHONEOS_DEPLOYMENT_TARGET = @DEPLOYMENT_TARGET@;
				SDKROOT = iphoneos;
			};
			name = Debug;
		};
		A1000000000000000000000C = {
			isa = XCBuildConfiguration;
			buildSettings = {
				CODE_SIGN_STYLE = Automatic;
				DEVELOPMENT_TEAM = "@TEAM@";
				GENERATE_INFOPLIST_FILE = YES;
				INFOPLIST_KEY_UILaunchScreen_Generation = YES;
				INFOPLIST_KEY_UIRequiresFullScreen = YES;
				PRODUCT_BUNDLE_IDENTIFIER = "@BUNDLE_ID@";
				PRODUCT_NAME = "$(TARGET_NAME)";
				TARGETED_DEVICE_FAMILY = "1,2";
			};
			name = Debug;
		};
		A10000000000000000000007 = {isa = XCConfigurationList; buildConfigurations = (A1000000000000000000000C, ); defaultConfigurationIsVisible = 0; defaultConfigurationName = Debug; };
		A1000000000000000000000A = {isa = XCConfigurationList; buildConfigurations = (A1000000000000000000000B, ); defaultConfigurationIsVisible = 0; defaultConfigurationName = Debug; };
	};
	rootObject = A10000000000000000000009;
}
"""

class Device:
    def __init__(self, entry: dict):
        hardware = entry.get("hardwareProperties", {})
        properties = entry.get("deviceProperties", {})
        self.identifier = entry.get("identifier")
        self.udid = hardware.get("udid")
        self.name = properties.get("name", hardware.get("marketingName", "iPhone"))
        self.os_version = properties.get("osVersionNumber", "")
        self.developer_mode = properties.get("developerModeStatus", "enabled")


class Signing:
    def __init__(self, identity: str, name: str, profile_path: Path, profile: dict):
        self.identity = identity
        self.name = name
        self.profile_path = profile_path
        self.profile = profile

    @property
    def team(self) -> str:
        return self.profile.get("TeamIdentifier", [""])[0]

    def entitlements(self, bundle_id: str) -> dict:
        granted = self.profile.get("Entitlements", {})
        return {
            "application-identifier": f"{self.team}.{bundle_id}",
            "com.apple.developer.team-identifier": self.team,
            "get-task-allow": bool(granted.get("get-task-allow", True)),
        }


def capture(cmd) -> str:
    return subprocess.run([str(part) for part in cmd], capture_output=True, text=True).stdout


def find_identities() -> list[tuple[str, str]]:
    output = capture(["security", "find-identity", "-v", "-p", "codesigning"])
    return [match.groups() for match in map(DEVELOPMENT_IDENTITY_PATTERN.match, output.splitlines()) if match]


def read_profile(path: Path) -> dict:
    decoded = subprocess.run(["security", "cms", "-D", "-i", str(path)], capture_output=True)
    if decoded.returncode != 0:
        return {}
    try:
        return plistlib.loads(decoded.stdout)
    except Exception:
        return {}


def installed_profiles() -> list[tuple[Path, dict]]:
    profiles = []
    for directory in PROFILE_DIRECTORIES:
        for path in sorted(directory.glob("*.mobileprovision")) if directory.exists() else []:
            profile = read_profile(path)
            if profile:
                profiles.append((path, profile))
    return profiles


def profile_app_id(profile: dict) -> str:
    app_id = profile.get("Entitlements", {}).get("application-identifier", "")
    return app_id.split(".", 1)[1] if "." in app_id else app_id


def profile_matches(profile: dict, bundle_id: str) -> bool:
    pattern = profile_app_id(profile)
    return pattern == bundle_id or (pattern.endswith("*") and bundle_id.startswith(pattern[:-1]))


def profile_is_usable(profile: dict, identity: str, device: Device = None) -> bool:
    expiration = profile.get("ExpirationDate")
    if expiration is not None and expiration.replace(tzinfo=timezone.utc) < datetime.now(timezone.utc):
        return False
    if "iOS" not in profile.get("Platform", ["iOS"]):
        return False
    if device is not None and not profile.get("ProvisionsAllDevices") and device.udid not in profile.get("ProvisionedDevices", []):
        return False
    certificates = {hashlib.sha1(der).hexdigest().upper() for der in profile.get("DeveloperCertificates", [])}
    return identity in certificates


def xcode_teams() -> tuple[list[dict], str]:
    exported = subprocess.run(["defaults", "export", "com.apple.dt.Xcode", "-"], capture_output=True).stdout
    preferences = plistlib.loads(exported) if exported else {}
    teams = {}
    for entries in (preferences.get("IDEProvisioningTeamByIdentifier") or {}).values():
        for team in entries:
            teams[team.get("teamID")] = team
    return list(teams.values()), preferences.get("IDEProvisioningTeamManagerLastSelectedTeamID")


def resolve_team(requested: str = None) -> str:
    if requested:
        return requested
    teams, last_selected = xcode_teams()
    if not teams:
        raise JobError("No Apple ID is signed in to Xcode. Add yours in Xcode > Settings > Accounts; "
                       "the packager uses it to create your certificate and provisioning profile.", 1)
    if len(teams) == 1:
        return teams[0]["teamID"]
    if last_selected in {team["teamID"] for team in teams}:
        return last_selected
    listing = ", ".join(f"{team.get('teamName', '?')} ({team['teamID']})" for team in teams)
    raise JobError(f"Your Apple ID belongs to several teams: {listing}. Pick one with --team.", 1)


def default_bundle_id(team: str) -> str:
    return f"com.artifact.{team.lower()}"


def list_devices() -> list[dict]:
    with tempfile.TemporaryDirectory(prefix="artifact-devicectl-") as temporary:
        output = Path(temporary) / "devices.json"
        subprocess.run(["xcrun", "devicectl", "list", "devices", "--json-output", str(output)], capture_output=True)
        return json.loads(output.read_text()).get("result", {}).get("devices", []) if output.exists() else []


def is_physical_phone(entry: dict) -> bool:
    hardware = entry.get("hardwareProperties", {})
    transport = entry.get("connectionProperties", {}).get("transportType")
    return hardware.get("platform") == "iOS" and hardware.get("reality") != "simulated" and transport in ("wired", "localNetwork")


def pair(entry: dict):
    name = entry.get("deviceProperties", {}).get("name", "your iPhone")
    print(f"> Pairing with {name}: unlock it and tap Trust when it asks")
    result = subprocess.run(["xcrun", "devicectl", "manage", "pair", "--device", entry.get("identifier")], capture_output=True, text=True)
    if result.returncode != 0:
        raise JobError(f"Could not pair with {name}. Unlock it, tap Trust on the 'Trust This Computer?' prompt and run again.\n"
                       + (result.stderr or result.stdout).strip(), 1)


def connected_device() -> Device:
    phones = [entry for entry in list_devices() if is_physical_phone(entry)]
    phones.sort(key=lambda entry: entry.get("connectionProperties", {}).get("transportType") != "wired")
    if not phones:
        return None
    entry = phones[0]
    if entry.get("connectionProperties", {}).get("pairingState") != "paired":
        pair(entry)
        entry = next((candidate for candidate in list_devices() if candidate.get("identifier") == entry.get("identifier")), entry)
    device = Device(entry)
    if device.developer_mode == "disabled":
        raise JobError(f"Developer Mode is off on {device.name}. Turn it on in Settings > Privacy & Security > "
                       "Developer Mode, let the phone restart, and run again.", 1)
    return device


def find_signing(bundle_id: str, device: Device = None, sign_key: str = None, profile_path: str = None) -> Signing:
    identities = find_identities()
    if sign_key:
        identities = [(sha, name) for sha, name in identities if sign_key.upper() == sha or sign_key in name]
    candidates = [(Path(profile_path), read_profile(Path(profile_path)))] if profile_path else installed_profiles()
    for sha, name in identities:
        for path, profile in candidates:
            if profile_matches(profile, bundle_id) and profile_is_usable(profile, sha, device):
                return Signing(sha, name, path, profile)
    return None


def write_stub_project(directory: Path, team: str, bundle_id: str) -> Path:
    if directory.exists():
        shutil.rmtree(directory)
    project = directory / f"{STUB_NAME}.xcodeproj"
    project.mkdir(parents=True)
    (directory / "main.c").write_text("int main(void) {\n    return 0;\n}\n")
    (project / "project.pbxproj").write_text(STUB_PROJECT.replace("@NAME@", STUB_NAME).replace("@TEAM@", team)
                                             .replace("@BUNDLE_ID@", bundle_id).replace("@DEPLOYMENT_TARGET@", DEPLOYMENT_TARGET))
    return project


def provision(directory: Path, team: str, bundle_id: str, device: Device):
    project = write_stub_project(directory, team, bundle_id)
    destination = f"platform=iOS,id={device.udid}" if device else "generic/platform=iOS"
    print(f"> Provisioning {bundle_id} for team {team}" + (f" on {device.name}" if device else ""))
    result = subprocess.run([
        "xcodebuild", "-project", str(project), "-scheme", STUB_NAME, "-configuration", "Debug",
        "-destination", destination, "-derivedDataPath", str(directory / "DerivedData"),
        "-allowProvisioningUpdates", "-allowProvisioningDeviceRegistration", "build",
    ], capture_output=True, text=True)
    if result.returncode != 0:
        lines = (result.stdout + result.stderr).splitlines()
        errors = [line for line in lines if "error" in line.lower()] or lines[-15:]
        hint = "" if device else "\nNo iPhone was connected. Plug it in with a cable, unlock it and trust this Mac, then run again."
        raise JobError("Xcode could not create the signing assets:\n  " + "\n  ".join(dict.fromkeys(errors)) + hint, 1)


def ensure_signing(directory: Path, args) -> tuple[Signing, str, Device]:
    device = connected_device()
    explicit_profile = getattr(args, "provisioning_profile", None)
    if explicit_profile:
        profile = read_profile(Path(explicit_profile))
        bundle_id = getattr(args, "bundle_id", None) or profile_app_id(profile)
        signing = find_signing(bundle_id, device, getattr(args, "sign_key", None), explicit_profile)
        if signing is None:
            raise JobError(f"{explicit_profile} does not cover {bundle_id}"
                           + (f" on {device.name}" if device else "") + " with any of your development certificates.", 1)
        return signing, bundle_id, device

    team = resolve_team(getattr(args, "team", None))
    bundle_id = getattr(args, "bundle_id", None) or default_bundle_id(team)
    signing = find_signing(bundle_id, device, getattr(args, "sign_key", None))
    if signing is None:
        provision(directory, team, bundle_id, device)
        signing = find_signing(bundle_id, device, getattr(args, "sign_key", None))
    if signing is None:
        raise JobError(f"Xcode finished, but no development certificate and profile for {bundle_id} were found.", 1)
    return signing, bundle_id, device
