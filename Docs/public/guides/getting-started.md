# Getting Started

Artifact Engine is a modular C++20 game engine. All tooling runs through the `artifact` CLI,
which is installed from the engine's `ArtifactSDK/` directory.

## Prerequisites

- Python 3.10 or newer
- Git

Everything else — the C++ toolchain, CMake, Ninja and the Vulkan SDK — is installed by
`artifact setup` below.

## Install the SDK

Clone the engine and install the CLI into a virtual environment:

```bash
git clone https://github.com/YanSchw/ArtifactEngine.git
cd ArtifactEngine
python3 -m venv venv
source venv/bin/activate          # Windows: venv\Scripts\Activate.ps1
pip install -e ./ArtifactSDK/.
```

The build toolchain (`cmake`, `ninja`) is pulled in as pip dependencies, so nothing else
needs to be installed system-wide.

## Install the development dependencies

```bash
artifact setup
```

This checks what the machine needs to build the engine and reports it:

```
Development dependencies for MacOS

✓ Xcode Command Line Tools  Apple clang version 17.0.0
✓ CMake                     4.0.2
✓ Ninja                     1.11.1
✗ Vulkan SDK                no SDK in $VULKAN_SDK or ~/VulkanSDK

Run `artifact setup vulkan` to install the missing dependencies.
Or run `artifact setup all` to install all dependencies.
```

`artifact setup vulkan` installs a single dependency, `artifact setup all` everything that
is missing. The dependencies differ per development platform — the C++ toolchain is
AppleClang on macOS, the Visual Studio 2022 build tools on Windows and GCC on Linux, and
Linux additionally needs the X11/Wayland development packages GLFW builds against. The
Vulkan SDK is installed from the command line rather than through LunarG's GUI installer.
Some installers need administrator rights and will ask for them.

## Create a project

```bash
artifact create project MyGame
cd MyGame
artifact run
```

`artifact create project` scaffolds a standalone project with its own `Modules/` and
`Content/` directories, initializes a git repository and generates the IDE project files.
`artifact run` builds the Dev configuration and launches the editor.

## Your first reflected class

```bash
artifact create class MyNode Node --module MyGame
```

This generates a header/source pair wired into the reflection system:

```cpp
#pragma once
#include "GameFramework/Node.h"
#include "MyNode.gen.h"

class MyNode : public Node {
public:
    ARTIFACT_CLASS();
    
    PROPERTY()
    float Health = 100.0f;
};
```

Reflected `PROPERTY()` members are serialized automatically and show up in the editor's
details panel. See the [API Reference](/api) for everything the engine ships with.

## Next steps

- [Building](/guides/building) — targets, configurations, cooking and packaging
- [Code Style](/guides/code-style) — conventions used across the codebase
