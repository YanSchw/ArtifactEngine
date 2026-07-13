# Getting Started

Artifact Engine is a modular C++20 game engine. All tooling runs through the `artifact` CLI,
which is installed from the engine's `ArtifactSDK/` directory.

## Prerequisites

- Python 3.10 or newer
- A C++20 toolchain (Xcode on macOS, Visual Studio 2022 on Windows)
- Git

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
