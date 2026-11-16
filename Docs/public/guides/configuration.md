# Configuration

Engine and game settings are **config vars**: flat, named string values such as `CapFPS` or
`DefaultScene`, read with `EngineConfig::GetConfigVar<T>("Name")`. The project's `.ini` files,
environment variables and the command line can all set them.

## Load order

Each layer overrides the ones before it:

| # | Source | Example |
| --- | --- | --- |
| 1 | Defaults declared in code | `int32_t CapFPS = 60;` on `RenderingSettings` |
| 2 | Frozen values baked into a packaged binary | see [Freezing](#freezing-config-vars-for-packaging) |
| 3 | The project's `.ini` files, in file name order | `CapFPS=144` |
| 4 | The user's `.ini` files, in file name order | `CapFPS=75` |
| 5 | Environment variables prefixed with `AE_` | `AE_CapFPS=30` |
| 6 | Command line arguments | `-CapFPS=30` |

| | Project config | User config |
| --- | --- | --- |
| Development | `<Project>/Config/` | `<Project>/Saved/Config/` |
| Packaged | `Content/Config/`, shipped by the packager (read-only) | `<user config dir>/<ProjectName>/Config/` |

The user config dir is `~/Library/Application Support` on macOS and iOS, the roaming AppData
folder on Windows and `$XDG_CONFIG_HOME` (or `~/.config`) on Linux. On Web it lives in memory
only, so nothing written there survives a reload yet.

## Ini files

```ini
; Config/Engine.ini
[Rendering]
CapFPS=144
RenderPipelineClass=ArtifactRenderPipeline

[Audio]
AudioMaxVoices=64
```

Sections only group keys for readers: every key is a config var of the same name, whichever
section or file it is in. Lines starting with `;` or `#` are comments. Strings, UUIDs, enums
and class names are written as they are; numbers and booleans plainly; arrays and structs as
JSON.

## Project Settings

**File → Project Settings** edits the project's config in the editor. Each group shown there
is a `ProjectSettings` subclass: every `PROPERTY` is the config var of the same name and its
initializer is the default.

```cpp
#include "Core/ProjectSettings.h"
#include "MyGameSettings.gen.h"

class MyGameSettings : public ProjectSettings {
public:
    ARTIFACT_CLASS();

    PROPERTY(Range = (1, 64))
    int32_t MaxPlayers = 8;

    PROPERTY(AssetType = Scene)
    UUID LobbyScene;
};
```

Subclassing is the registration. The group is saved to `Config/Game.ini` under
`[MyGame]` (the class name without `Settings`); override `GetConfigFile()` and `GetSection()`
to change either. `PROPERTY(AssetType = ...)` turns a `UUID` into an asset picker.

The editor only writes values that differ from the default, and edits apply to the running
editor right away. At runtime `ProjectSettings::Get<MyGameSettings>()` returns an instance that
always holds the current config var values, so game code can read settings typed:

```cpp
const int32_t maxPlayers = ProjectSettings::Get<MyGameSettings>()->MaxPlayers;
```

## Freezing config vars for packaging

Some values should be editable while developing but locked in a shipped game, like the
default scene. Click the lock next to a setting in Project Settings to freeze it. Frozen names
are listed in `Engine.ini`:

```ini
[Packaging]
FrozenConfigVars=["DefaultScene","RenderPipelineClass"]
```

Freezing changes nothing in development builds. When packaging:

- the frozen values are compiled into the binary,
- they are removed from the `.ini` files the package ships,
- packaged builds ignore any attempt to change them from an `.ini` file, an environment
  variable or the command line, and log a warning instead.

A frozen var without a value in any `.ini` file keeps the default of the packaged build.

`ProjectName` is always frozen: it names the folder the user config lives in, so a shipped
`.ini` must not be able to move it. Set it per project in Project Settings → Game. The
packager also names the app after it (`Dist/<ProjectName>.app`, `<ProjectName>.exe`, the
AppImage and the Web page title).
