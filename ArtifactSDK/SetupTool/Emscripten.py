"""The emscripten SDK, which is what builds the Web target.

Unlike the other dependencies this one is not a property of the host: any development platform can
cross-compile to the browser, so it is offered everywhere. emsdk installs entirely from the command
line and keeps everything inside its own directory, so it goes to a fixed place under the user's
home rather than into the system.
"""

import os
from pathlib import Path

from SetupTool.Dependency import CheckResult, Dependency, SetupError
from SetupTool.Process import command_output, first_line, run_command

EMSDK_REPOSITORY = "https://github.com/emscripten-core/emsdk.git"
EMSDK_DIRECTORY = Path.home() / ".artifact" / "emsdk"


def get_emsdk_root():
    """The emsdk to build with: the one EMSDK points at, else the one `artifact setup` installed."""
    candidates = []
    if os.environ.get("EMSDK"):
        candidates.append(Path(os.environ["EMSDK"]))
    candidates.append(EMSDK_DIRECTORY)

    for root in candidates:
        if (root / "upstream" / "emscripten" / "emcc.py").exists():
            return root
    return None


def get_toolchain_file(root: Path) -> Path:
    return root / "upstream" / "emscripten" / "cmake" / "Modules" / "Platform" / "Emscripten.cmake"


def _find_node_bin(root: Path):
    node_root = root / "node"
    if not node_root.is_dir():
        return None
    for version in sorted(node_root.iterdir()):
        if (version / "bin").is_dir():
            return version / "bin"
    return None


def get_build_environment(env: dict) -> dict:
    """Add emscripten to an environment, the way emsdk_env would."""
    root = get_emsdk_root()
    if root is None:
        raise SetupError("No emscripten SDK found. Run `artifact setup emscripten` to install one.")

    emscripten = root / "upstream" / "emscripten"
    paths = [str(emscripten)]

    node_bin = _find_node_bin(root)
    if node_bin is not None:
        paths.append(str(node_bin))
        env["EMSDK_NODE"] = str(node_bin / "node")

    env["EMSDK"] = str(root)
    env["EM_CONFIG"] = str(root / ".emscripten")
    env["PATH"] = os.pathsep.join(paths + [env.get("PATH", "")])
    return env


class EmscriptenSDK(Dependency):
    key = "emscripten"
    name = "Emscripten SDK"
    aliases = ("emsdk", "web", "wasm")

    def check(self) -> CheckResult:
        root = get_emsdk_root()
        if root is None:
            return CheckResult.missing(f"not found (looked in $EMSDK and {EMSDK_DIRECTORY})")

        version = first_line(command_output(["emcc", "--version"], env=get_build_environment(os.environ.copy())))
        return CheckResult.found(version or str(root))

    def install(self):
        script = EMSDK_DIRECTORY / ("emsdk.bat" if os.name == "nt" else "emsdk")

        if script.exists():
            run_command(["git", "-C", str(EMSDK_DIRECTORY), "pull", "--ff-only"])
        else:
            EMSDK_DIRECTORY.parent.mkdir(parents=True, exist_ok=True)
            run_command(["git", "clone", "--depth", "1", EMSDK_REPOSITORY, str(EMSDK_DIRECTORY)])

        run_command([str(script), "install", "latest"], cwd=str(EMSDK_DIRECTORY))
        run_command([str(script), "activate", "latest"], cwd=str(EMSDK_DIRECTORY))
