"""The .NET SDK, which builds the Scripting modules and hosts them in the editor.

Microsoft's dotnet-install script puts a private SDK under the user's home, so no admin rights are
needed. An SDK that is already installed system-wide is used as long as it is recent enough.
"""

import os
import shutil
import sys
from pathlib import Path

from SetupTool.Dependency import CheckResult, Dependency, SetupError
from SetupTool.Download import download_file
from SetupTool.Process import command_output, run_command

CHANNEL = "10.0"
MINIMUM_MAJOR = 10
DOTNET_DIRECTORY = Path.home() / ".artifact" / "dotnet"
EXECUTABLE = "dotnet.exe" if sys.platform == "win32" else "dotnet"
INSTALL_SCRIPT_URL = "https://dot.net/v1/dotnet-install." + ("ps1" if sys.platform == "win32" else "sh")


def _candidates() -> list[Path]:
    candidates = []
    if os.environ.get("DOTNET_ROOT"):
        candidates.append(Path(os.environ["DOTNET_ROOT"]) / EXECUTABLE)
    candidates.append(DOTNET_DIRECTORY / EXECUTABLE)
    if shutil.which("dotnet"):
        candidates.append(Path(shutil.which("dotnet")).resolve())
    return [candidate for candidate in dict.fromkeys(candidates) if candidate.is_file()]


def _version(dotnet: Path) -> str:
    return command_output([dotnet, "--version"]) or ""


def _major(version: str) -> int:
    head = version.split(".")[0]
    return int(head) if head.isdigit() else 0


def find_dotnet():
    for candidate in _candidates():
        if _major(_version(candidate)) >= MINIMUM_MAJOR:
            return candidate
    return None


class DotNetSDK(Dependency):
    key = "dotnet"
    name = ".NET SDK"
    aliases = ("csharp", "net", ".net")

    def check(self) -> CheckResult:
        dotnet = find_dotnet()
        if dotnet is not None:
            return CheckResult.found(f"{_version(dotnet)}  {dotnet.parent}")
        found = [f"{_version(candidate) or '?'} at {candidate.parent}" for candidate in _candidates()]
        if found:
            return CheckResult.missing(f"needs {MINIMUM_MAJOR}.0 or newer, found {', '.join(found)}")
        return CheckResult.missing(f"not found (looked in $DOTNET_ROOT, {DOTNET_DIRECTORY} and PATH)")

    def install(self):
        script = DOTNET_DIRECTORY.parent / Path(INSTALL_SCRIPT_URL).name
        download_file(INSTALL_SCRIPT_URL, script, "Downloading dotnet-install")
        if sys.platform == "win32":
            run_command(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", script,
                         "-Channel", CHANNEL, "-InstallDir", DOTNET_DIRECTORY, "-NoPath"])
        else:
            run_command(["bash", script, "--channel", CHANNEL, "--install-dir", DOTNET_DIRECTORY, "--no-path"])
        if find_dotnet() is None:
            raise SetupError(f"dotnet-install finished, but no .NET {MINIMUM_MAJOR} SDK was found in {DOTNET_DIRECTORY}")
