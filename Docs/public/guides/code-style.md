# Code Style

Conventions used throughout the engine. `artifact lint` enforces the mechanical rules and
runs in CI on every push.

## Formatting

- Indent with **spaces** — tab characters fail lint.
- Keep reflected type declarations in the conventional shape the HeaderTool expects:
  `class MyType : public Parent {` on one line.
- Every header needs a unique filename; duplicate basenames are a hard error in the
  code generator.

## Naming

- Function parameters are prefixed with `In`: `void SetName(const String& InName)`.
- File-static members are prefixed `s_`, member variables `m_`.

## Containers and strings

Prefer the engine's own types from `EngineCore/Common/` over raw STL in engine code:

```cpp
Array<SharedObjectPtr<Asset>> LoadedAssets;
Map<String, Class> ClassRegistry;
String DisplayName = "Artifact";
```

## Logging and asserts

Use the engine macros with `{0}`-style format arguments:

```cpp
AE_INFO("Loaded {0} assets in {1} ms", count, elapsed);
AE_WARN("Missing content mount '{0}'", key);
AE_ASSERT(instance != nullptr, "Engine instance must exist");
```

## Comments

The code should speak for itself. Comment only for documentation purposes or when a
segment genuinely needs it — a non-obvious design choice a reader would otherwise
question. Never write comments that narrate a change; that's what git history is for.
