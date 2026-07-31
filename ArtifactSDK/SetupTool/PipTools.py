"""CMake and Ninja, which the SDK pulls into its virtual environment via pip."""

import re
import shutil
import sys
from pathlib import Path

from SetupTool.Dependency import CheckResult, Dependency
from SetupTool.Process import command_output, first_line, note, run_command


def _parse_version(text: str):
    match = re.search(r"(\d+(?:\.\d+)+)", text or "")
    return tuple(int(part) for part in match.group(1).split(".")) if match else None


def _in_sdk_environment(executable: str) -> bool:
    """Whether the tool sits next to the interpreter but is not on PATH.

    That is the shape of an SDK virtual environment that was never activated:
    the package is installed, yet the build's `cmake`/`ninja` calls would not
    resolve to it.
    """
    directory = Path(sys.executable).parent
    return any((directory / f"{executable}{suffix}").exists() for suffix in ("", ".exe"))


class PipTool(Dependency):
    """A build tool installed as a pip package into the active environment.

    Both CMake and Ninja are declared in the SDK's requirements, so they are
    normally already there; this covers the case of an SDK installed outside a
    virtual environment, or a system CMake too old for the build.
    """

    package = ""
    executable = ""
    minimum_version = ()

    def check(self) -> CheckResult:
        path = shutil.which(self.executable)
        if not path:
            if _in_sdk_environment(self.executable):
                return CheckResult.missing("installed, but not on PATH — activate the SDK virtual environment")
            return CheckResult.missing("not found")
        output = first_line(command_output([path, "--version"], combined=True))
        version = _parse_version(output)
        if version is None:
            return CheckResult.found(path)
        printable = ".".join(str(part) for part in version)
        if self.minimum_version and version < self.minimum_version:
            required = ".".join(str(part) for part in self.minimum_version)
            return CheckResult.missing(f"{printable} is too old, {required} or newer is required")
        return CheckResult.found(printable)

    def install(self):
        run_command([sys.executable, "-m", "pip", "install", "--upgrade", self.package])
        if shutil.which(self.executable) is None:
            note(f"{self.name} was installed but is not on PATH — activate the SDK virtual environment.")


class CMake(PipTool):
    key = "cmake"
    name = "CMake"
    package = "cmake"
    executable = "cmake"
    # The build uses the "Ninja Multi-Config" generator, added in CMake 3.17.
    minimum_version = (3, 17)


class Ninja(PipTool):
    key = "ninja"
    name = "Ninja"
    package = "ninja"
    executable = "ninja"
    minimum_version = (1, 10)
