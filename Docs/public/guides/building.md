# Building

Everything goes through the `artifact` CLI — there is no `make` or raw `cmake` workflow.
Each build regenerates `CMakeLists.txt` and the reflection `.gen` files before compiling,
so generated files are never edited by hand.

## Commands

```bash
artifact setup          # check this machine's dev dependencies ('all' or a name installs them)
artifact build [--target Win64|MacOS|Linux|Web|iOS] [--configuration Debug|Dev|Dist] [--clean]
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
`artifact package` cooks, builds `Dist` and bundles the result.

## iOS

iOS packages are built on a Mac with Xcode and the iOS component of the LunarG Vulkan SDK. The app links MoltenVK directly and embeds `MoltenVK.framework` in `Artifact.app/Frameworks`.

```bash
artifact package --target iOS --simulator --install   # booted iOS Simulator, ad-hoc signed
artifact package --target iOS --install               # connected iPhone, development signed
```

Device builds sign themselves through the Apple ID you are signed in to in Xcode
(Settings → Accounts). If no development certificate and profile cover the bundle identifier
and the connected iPhone yet, the packager builds a stub app with
`xcodebuild -allowProvisioningUpdates`, which creates the certificate, registers the phone and
the identifier, and downloads the profile. The identifier defaults to `com.artifact.<team id>`;
`--bundle-id`, `--team` and `--provisioning-profile` override it. The phone needs Developer
Mode on (Settings → Privacy & Security). Profiles of a free Personal Team expire after 7 days
and are renewed on the next package run.
