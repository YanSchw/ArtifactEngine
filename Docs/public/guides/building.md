# Building

Everything goes through the `artifact` CLI — there is no `make` or raw `cmake` workflow.
Each build regenerates `CMakeLists.txt` and the reflection `.gen` files before compiling,
so generated files are never edited by hand.

## Commands

```bash
artifact build [--target Win64|MacOS|Linux] [--configuration Debug|Dev|Dist] [--clean]
artifact run            # builds (Dev) then launches Binaries/Artifact
artifact cook           # cooks Content/ into Build/Intermediate/Cooked
artifact package        # cooks + Dist build + platform bundle
artifact docs           # dumps the API reference JSON for this documentation site
artifact lint           # lints all .cpp/.h files
```

## Configurations

| Configuration | Purpose |
| --- | --- |
| `Debug` | Full debug information, no optimizations. |
| `Dev` | Optimized but with editor and debug tooling. The default. |
| `Dist` | Shipping build, used by `artifact package`. |

## Modules

The engine is split into modules — one directory under `Modules/` each, declared by a
`Module.json`:

```json
{
    "SourceDirectories": ["Source"],
    "ImportModules": ["EngineCore"]
}
```

`ImportModules` controls include-path propagation and link order. Projects resolve modules
from both the engine and their own `Modules/` directory. See the
[module list](/api) in the API reference.

## Cooking and packaging

`artifact cook` builds the engine, then relaunches the binary with
`-EngineClass=AssetCookerEngine` to convert source assets into their cooked runtime format.
`artifact package` cooks, builds `Dist` and bundles the result (macOS and Windows so far).
