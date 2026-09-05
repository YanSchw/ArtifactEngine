<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset=".github/artifact-logo-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset=".github/artifact-logo-light.svg">
    <img alt="Artifact Engine" src=".github/artifact-logo-dark.svg" width="360">
  </picture>

  [Website] | [Getting Started] | [Documentation] | [API Reference]

  [![Build](https://github.com/YanSchw/ArtifactEngine/actions/workflows/build.yml/badge.svg)](https://github.com/YanSchw/ArtifactEngine/actions/workflows/build.yml)
  [![Integrity Checks](https://github.com/YanSchw/ArtifactEngine/actions/workflows/integrity_checks.yml/badge.svg)](https://github.com/YanSchw/ArtifactEngine/actions/workflows/integrity_checks.yml)
  ![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
</div>

[Website]: https://artifact.schoessow.net/
[Getting Started]: https://artifact-docs.schoessow.net/guides/getting-started
[Documentation]: https://artifact-docs.schoessow.net/
[API Reference]: https://artifact-docs.schoessow.net/

---

Artifact is a modular game engine written in C++.

## Features
- Editor
- Rendering (RHI)
- Shader & Material Graph System
- User Interfaces
- Physics via JoltPhysics
- Networking

## Supported platforms

| Target | Graphics | Editor & Dev Environment | Packaged Runtime |
| --- | --- | --- | --- |
| **Windows** (x64) | Vulkan | ✔︎ | ✔︎ |
| **macOS** | Vulkan via MoltenVK |  ✔︎ | ✔︎ |
| **Linux** (X11 + Wayland) | Vulkan | ✔︎ | - (not yet) |
| **Web** (WebAssembly) | WebGL 2 | - | ✔︎ (static hosting, no cross-origin isolation) |

## Repository layout

| Path | Contents |
| --- | --- |
| `ArtifactSDK/` | The `artifact` CLI: build tool, header tool, linter, packager, setup tool |
| `Modules/` | The engine source code: one directory per module, each with a `Module.json` |
| `Content/` | Engine content: shaders, default materials, fonts, icons, etc. |
| `Docs/` | A documentation web app (Angular + TailwindCSS) |

## Project status

Artifact is still in an early version and under active development. The engine builds, the editor runs, and games package on all targets. Treat it as something to read, build and experiment with rather
than something to ship on.

## Documentation

Guides and a full API reference generated from the engine's own reflection data live at
[artifact-docs.schoessow.net](https://artifact-docs.schoessow.net/). Start with
[Getting Started](https://artifact-docs.schoessow.net/guides/getting-started), then
[Building](https://artifact-docs.schoessow.net/guides/building) for the CLI in depth and
[Code Style](https://artifact-docs.schoessow.net/guides/code-style) before opening a pull request.
