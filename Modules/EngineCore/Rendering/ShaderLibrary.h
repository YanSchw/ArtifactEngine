#pragma once
#include "CoreMinimal.h"
#include "CompiledShader.h"
#include "Platform/Platform.h"
#include "ShaderLibrary.gen.h"

/** Owns every Shader in the process and the GLSL front-end that produces them.
 *
 *  Shaders are addressed by key: a content-relative path ("/Shaders/Scene.glsl") for shaders that
 *  ship with the codebase, or a UUID string for shaders generated from an asset. Unpackaged builds
 *  preprocess and compile on first use; packaged builds load a cooked library at startup and every
 *  request is a lookup. */
class ShaderLibrary : public Object {
public:
    ARTIFACT_CLASS();
    ShaderLibrary() = delete;

    static constexpr const char* CookedFileName = "ShaderLibrary";

    static void Initialize();
    static void Shutdown();

    static SharedObjectPtr<class Shader> CreateShader(const String& InKey);
    static SharedObjectPtr<class Shader> Find(const String& InKey);
    static bool Reload(const String& InKey);

    /** Registers source for a key that has no file behind it, so generated shaders participate in
     *  live compilation and cooking exactly like the ones on disk. */
    static void RegisterSource(const String& InKey, const String& InSource);

    static Array<String> GetLoadedKeys();

    static bool Cook(const String& InOutputDirectory, PlatformType InTargetPlatform);

private:
    static bool CompileForKey(const String& InKey, ShaderAPI InAPI, CompiledShader& OutCompiled, String& OutError);
    static bool LoadCookedLibrary();
    static Array<String> DiscoverShaderKeys();
};
