# ArtifactSDK

## Install

Requirements:
- Python 3.7+

### Windows
```powershell
py -m venv venv
venv\Scripts\Activate.ps1
pip install -e .\ArtifactSDK\.
```

### macOS / Linux
```sh
python3 -m venv venv
source venv/bin/activate
pip install -e ./ArtifactSDK/.
```

## Run SDK
```
artifact --help
```

```
❯ artifact --help
usage: artifact [-h]
                {generate,build,run,cook,package,docs,lint,setup,version,location,create} ...

Artifact Engine Build Tool

positional arguments:
  {generate,build,run,cook,package,docs,lint,setup,version,location,create}
    generate            Generate project files (CMake + native IDE project)
                        without building
    build               Build the engine
    run                 Run the engine
    cook                Cook assets
    package             Package project
    docs                Dump reflection data (classes, structs, enums,
                        modules) as JSON for the Docs frontend
    lint                Lint C++/Header files
    setup               Check (and install) the development dependencies of
                        this machine
    version             Show engine version
    location            Print the engine path to the terminal
    create              Scaffold a project, module, or reflected type

options:
  -h, --help            show this help message and exit
```

## Development dependencies

`artifact setup` reports what this machine is missing and installs it without opening any
GUI installer:

```
❯ artifact setup

Development dependencies for MacOS

✓ Xcode Command Line Tools  Apple clang version 17.0.0
✓ CMake                     4.0.2
✓ Ninja                     1.11.1
✗ Vulkan SDK                no SDK in $VULKAN_SDK or ~/VulkanSDK

Run `artifact setup vulkan` to install the missing dependencies.
Or run `artifact setup all` to install all dependencies.
```

Which dependencies exist depends on the machine, not on the build target:

| Dependency | macOS | Windows | Linux |
| --- | --- | --- | --- |
| `toolchain` | Xcode Command Line Tools | Visual Studio 2022 C++ build tools | GCC (`build-essential`) |
| `cmake` / `ninja` | pip packages in the SDK venv | same | same |
| `windowing` | — | — | X11 + Wayland dev packages for GLFW |
| `vulkan` | LunarG SDK + loader in `/usr/local` | LunarG SDK + `VULKAN_SDK` | LunarG apt repo, else distro packages |

Without arguments the command only checks and exits non-zero when something is missing, so
it works as a CI gate. Installing needs elevated rights for some dependencies (sudo on
macOS/Linux, UAC on Windows) and prompts for it inline. `--force` reinstalls a dependency
that is already present.

`artifact generate` writes the CMake files, reflection code, and a native IDE
project (a Visual Studio solution on Windows, an Xcode project on macOS) at
the project root. Building from the IDE just activates this venv and shells 
out to `artifact build`.