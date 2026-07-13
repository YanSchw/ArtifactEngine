`Asset` is the base class of everything that can live in a content directory — textures,
meshes, fonts and vector images all derive from it. Assets are identified by a `UUID` and
loaded on demand through the streaming system.

## Streaming

Assets are not loaded eagerly. Acquire an `AssetStreamHandle` to pin an asset in memory:

```cpp
AssetStreamHandle Handle = AssetStreamHandle(MyAsset);
if (Texture2D* Texture = Cast<Texture2D>(Handle.Get())) {
    // The asset stays resident while the handle is alive.
}
```

The `StreamType` property controls the policy: `Referenced` assets load when something
points at them, `Manual` assets only load through explicit handles, and `AlwaysLoaded`
assets are resident for the lifetime of the engine.

## Locating asset files

Content is mounted from several roots (engine, project and per-module mounts). Use
`EngineConfig::ResolveContentPath(relPath)` to find an asset file across all mounts instead
of assuming a single content directory.
