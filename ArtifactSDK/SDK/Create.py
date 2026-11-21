import os
import sys
import shutil
import subprocess

from colorama import Fore, Style

from SDK.Paths import get_engine_path, get_project_path
from SDK.Util import smart_open
from BuildTool.Module import ArtifactModule

# Known reflected bases; any other parent falls back to "<Parent>.h".
KNOWN_BASE_INCLUDES = {
    "Object": "Object/Object.h",
    "Node": "GameFramework/Node.h",
    "Node3D": "GameFramework/Node3D.h",
    "Component": "GameFramework/Component.h",
    "UINode": "GameFramework/UINode.h",
}

CLASS_KIND_DEFAULT_PARENT = {
    "class": "Object",
    "node": "Node",
    "component": "Component",
}

def _fail(message: str):
    print(f"{Fore.RED}{message}{Style.RESET_ALL}")
    sys.exit(1)

def _created(path: str):
    print(f"{Fore.GREEN}Created{Style.RESET_ALL} {path}")

def _base_include_for(parent: str) -> str:
    return KNOWN_BASE_INCLUDES.get(parent, f"{parent}.h")

def _list_modules(project_path: str) -> list[str]:
    modules_dir = f"{project_path}/Modules"
    if not os.path.isdir(modules_dir):
        return []
    return sorted(
        name for name in os.listdir(modules_dir)
        if os.path.isfile(f"{modules_dir}/{name}/Module.json")
    )

def _resolve_module(project_path: str, module_name: str | None) -> str:
    available = _list_modules(project_path)
    if module_name is not None:
        if module_name not in available:
            _fail(f"Module '{module_name}' not found in {project_path}/Modules")
        return module_name
    if len(available) == 0:
        _fail("No modules found. Create one first with 'artifact create module <name>'.")
    if len(available) > 1:
        _fail(f"Multiple modules exist ({', '.join(available)}); pick one with --module <name>.")
    return available[0]

def _module_source_dir(project_path: str, module_name: str) -> str:
    """Directory inside the module where new source files should live."""
    module_dir = f"{project_path}/Modules/{module_name}"
    module = ArtifactModule.load_from_json(module_dir)
    if module.SourceDirectories:
        return f"{module_dir}/{module.SourceDirectories[0]}"
    return module_dir

def _assert_unique_header(project_path: str, engine_path: str, name: str):
    """HeaderTool requires globally-unique header basenames, so refuse a clash up front."""
    target = f"{name}.h"
    for root_path in {project_path, engine_path}:
        modules_dir = f"{root_path}/Modules"
        for dirpath, _dirs, filenames in os.walk(modules_dir):
            if target in filenames:
                existing = os.path.join(dirpath, target)
                _fail(f"A header named '{target}' already exists ({existing}). "
                      f"Header basenames must be unique.")

def _write_class_files(target_dir: str, name: str, parent: str):
    base_include = _base_include_for(parent)
    header = f"""#pragma once
#include "{base_include}"
#include "{name}.gen.h"

class {name} : public {parent} {{
public:
    ARTIFACT_CLASS();
}};
"""
    source = f"""#include "{name}.h"
"""
    header_path = f"{target_dir}/{name}.h"
    source_path = f"{target_dir}/{name}.cpp"
    with smart_open(header_path) as f:
        f.write(header)
    with smart_open(source_path) as f:
        f.write(source)
    _created(header_path)
    _created(source_path)

def _write_struct_files(target_dir: str, name: str):
    header = f"""#pragma once
#include "{name}.gen.h"

struct {name} {{
    ARTIFACT_STRUCT();
}};
"""
    header_path = f"{target_dir}/{name}.h"
    with smart_open(header_path) as f:
        f.write(header)
    _created(header_path)

def create_reflected_type(kind: str, name: str, parent: str | None, module_name: str | None):
    project_path = get_project_path()
    engine_path = get_engine_path()

    resolved_module = _resolve_module(project_path, module_name)
    target_dir = _module_source_dir(project_path, resolved_module)
    os.makedirs(target_dir, exist_ok=True)

    _assert_unique_header(project_path, engine_path, name)

    if kind == "struct":
        if parent is not None:
            _fail("Structs cannot declare a parent class.")
        _write_struct_files(target_dir, name)
    else:
        resolved_parent = parent if parent is not None else CLASS_KIND_DEFAULT_PARENT[kind]
        _write_class_files(target_dir, name, resolved_parent)

    print(f"{Fore.CYAN}Run 'artifact build' to generate reflection code for {name}.{Style.RESET_ALL}")

def create_module(name: str, import_modules: list[str] | None = None, module_type: str = "Native"):
    project_path = get_project_path()
    module_dir = f"{project_path}/Modules/{name}"
    if os.path.exists(module_dir):
        _fail(f"Module '{name}' already exists at {module_dir}")

    if module_type == "Scripting":
        module = ArtifactModule(name=name, path=module_dir, ImportModules=import_modules, SourceDirectories=["Source"], Type="Scripting")
    else:
        module = ArtifactModule(
            name=name,
            path=module_dir,
            ImportModules=import_modules if import_modules is not None else ["EngineCore"],
            SourceDirectories=["Source"],
            IncludePaths=["Source"],
            ExportIncludePaths=["Source"],
        )
    os.makedirs(f"{module_dir}/Source", exist_ok=True)
    module.write_to_json()
    _created(f"{module_dir}/Module.json")
    return module_dir

PROJECT_GITIGNORE = """# Python
.venv
venv
__pycache__
*.pyc
*egg-info

.DS_Store
.vscode
.vs

CMakeLists.txt
Dist/
Build/
Binaries/
Intermediate/
Saved/
*.log

# Artifact SDK venv activation
ActivateSDK.ps1
activateSDK.bat
activateSDK.sh

*.sln
*.vcxproj
*.vcxproj.filters
*.vcxproj.user
*.xcodeproj/
"""

def _project_readme(name: str) -> str:
    return f"""# {name}

This project was generated with the [Artifact Engine](https://github.com/YanSchw/ArtifactEngine) SDK.

## Setup

The engine and the project live in separate directories. The engine owns the Python venv with the
Artifact SDK; the project only links to it.

### 1. Install the engine and its SDK

Clone the engine (or unpack a release zip) next to this project, then create the SDK venv inside it:

```sh
git clone https://github.com/YanSchw/ArtifactEngine.git
cd ArtifactEngine
python3 -m venv venv
source venv/bin/activate            # Windows: venv\\Scripts\\Activate.ps1
pip install -e ./ArtifactSDK/.
artifact setup all                  # installs CMake, Ninja, the Vulkan SDK, ...
```

### 2. Link this project to the engine

With the engine's venv active, link the project once after cloning it:

```sh
artifact link path/to/{name}
```

This writes `activateSDK.sh`, `ActivateSDK.ps1` and `activateSDK.bat` into the project.

### 3. Activate the SDK from the project

In any new shell, activate the engine's SDK from the project directory:

```sh
source ./activateSDK.sh             # macOS / Linux
. .\\ActivateSDK.ps1                 # Windows PowerShell
activateSDK.bat                     # Windows cmd
```

## Building

From this directory, with the SDK active:

```sh
artifact build                      # build (Dev by default, --configuration Debug|Dev|Dist)
artifact run                        # build Dev and run
artifact package                    # cook the content and package a shippable build
```

## Adding code

```sh
artifact create module MyModule     # a new module under Modules/
artifact create node MyNode         # a reflected Node in the project's module
artifact create class MyObject      # a reflected Object
```
"""

def create_project(name: str):
    parent_dir = get_project_path()
    engine_path = get_engine_path()
    project_dir = f"{parent_dir}/{name}"

    if os.path.exists(project_dir):
        _fail(f"'{project_dir}' already exists.")

    os.makedirs(project_dir)
    os.chdir(project_dir)

    subprocess.run(["git", "init", "-q"], check=True)

    with smart_open(f"{project_dir}/.gitignore") as f:
        f.write(PROJECT_GITIGNORE)
    _created(f"{project_dir}/.gitignore")
    with smart_open(f"{project_dir}/README.md") as f:
        f.write(_project_readme(name))
    _created(f"{project_dir}/README.md")

    # png_to_ico during generate needs a project icon to exist.
    os.makedirs(f"{project_dir}/Content/Icons", exist_ok=True)
    engine_icon = f"{engine_path}/Content/Icons/Icon.png"
    if os.path.exists(engine_icon):
        shutil.copyfile(engine_icon, f"{project_dir}/Content/Icons/Icon.png")
        _created(f"{project_dir}/Content/Icons/Icon.png")

    create_module("Game")
    create_reflected_type("node", "GameMode", parent=None, module_name="Game")

    subprocess.run(["git", "add", "-A"], check=True)
    subprocess.run(["git", "commit", "-q", "-m", "Initial project scaffolding"], check=True)

    print(f"{Fore.CYAN}Generating project files...{Style.RESET_ALL}")
    subprocess.run(["artifact", "generate"], check=False)

    print(f"{Fore.GREEN}Project '{name}' created at {project_dir}{Style.RESET_ALL}")

def link_project(path: str):
    project_path = os.path.abspath(path).replace("\\", "/")
    engine_path = get_engine_path()
    if not os.path.isdir(f"{project_path}/Modules"):
        _fail(f"'{project_path}' is not an Artifact project (no Modules/ directory).")
    if os.path.normpath(project_path) == os.path.normpath(engine_path):
        _fail("The engine does not need to be linked to itself.")

    generate_sdk_activation_scripts(project_path, engine_path)
    print(f"{Fore.GREEN}Linked{Style.RESET_ALL} {project_path} to the engine at {engine_path}")
    print(f"{Fore.CYAN}Activate the SDK from the project with: source {project_path}/activateSDK.sh{Style.RESET_ALL}")

def generate_sdk_activation_scripts(project_path: str, engine_path: str):
    """Emit activateSDK.{sh,bat}/ActivateSDK.ps1 that activate the engine's venv. """
    if os.path.normpath(project_path) == os.path.normpath(engine_path):
        return

    venv = sys.prefix.replace("\\", "/") if sys.prefix != sys.base_prefix else f"{engine_path}/venv"
    venv_win = venv.replace("/", "\\")

    activate_sh = f"{project_path}/activateSDK.sh"
    with smart_open(activate_sh) as f:
        f.write(f"""#!/bin/sh
# Generated by Artifact SDK. Source this to activate the engine's venv:
#   source ./activateSDK.sh
if [ -n "$ZSH_VERSION" ]; then
    case $ZSH_EVAL_CONTEXT in *:file*) _artifact_sourced=1 ;; esac
else
    (return 0 2>/dev/null) && _artifact_sourced=1
fi
if [ -z "$_artifact_sourced" ]; then
    echo "activateSDK.sh has to be sourced to activate the SDK in your shell:" >&2
    echo "  source {activate_sh}" >&2
    exit 1
fi
unset _artifact_sourced
. "{venv}/bin/activate"
""")
    os.chmod(activate_sh, 0o755)

    with smart_open(f"{project_path}/ActivateSDK.ps1") as f:
        f.write(f"""# Generated by Artifact SDK. Dot-source this to activate the engine's venv:
#   . .\\ActivateSDK.ps1
& "{venv}/Scripts/Activate.ps1"
""")

    with smart_open(f"{project_path}/activateSDK.bat") as f:
        f.write(f"""@echo off
REM Generated by Artifact SDK. Run this to activate the engine's venv.
call "{venv_win}\\Scripts\\activate.bat"
""")
