`Object` is the root of the reflection hierarchy — the class every reflected type
ultimately derives from. It is the one class that does **not** carry an `ARTIFACT_CLASS()`
macro, because there is nothing above it to derive from; it hand-implements the same
reflection surface (`GetClass`, `StaticClass`, `IsObjectOfClass`) that the generated
`.gen.h` fills in for its subclasses.

A type joins the hierarchy by publicly deriving from another reflected type and placing
`ARTIFACT_CLASS();` in its body:

```cpp
class Texture2D : public Asset {
    ARTIFACT_CLASS();
};
```

That macro expands to `GENERATED_BODY(__LINE__)`, which the HeaderTool fills in with the
class' `StaticClass`, `GetClass` override, and parent-class registration.

## Creating objects by class

Reflected types are constructed by class identity rather than with a direct `new`, so a
class named only at runtime (from a config var, a serialized asset, an editor dropdown) can
still be instantiated:

```cpp
Object* Obj = Object::Create(SomeClass);
Texture2D* Texture = Object::Create<Texture2D>();
```

`Create` looks the class name up in the allocator table that every
`RegisterArtifactClass<T>` populates at startup, so only default-constructible reflected
types can be produced this way.

## Type queries and casting

`Class` is a string-named handle. Query an instance's type with `IsA`, and downcast
type-safely with `Cast<To>()`, which returns `nullptr` when the object is not of the
requested class:

```cpp
if (Texture2D* Texture = Cast<Texture2D>(Obj)) {
    // Obj really is a Texture2D (or a subclass of it).
}

if (Obj->IsA<Asset>()) {
    // ...
}
```

`Class` also supports hierarchy walks — `GetParentClass`, `IsSubclassOf`,
`GetSubclassesOf`, and `GetAllClasses` — which is how the editor enumerates spawnable
types and how this documentation is generated.

## Ownership

Prefer `SharedObjectPtr<T>` for owning references to objects rather than raw pointers, so
lifetime is reference-counted rather than manual.
