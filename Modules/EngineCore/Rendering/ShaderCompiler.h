#pragma once
#include "CoreMinimal.h"
#include "CompiledShader.h"
#include "ShaderSource.h"
#include "Platform/Platform.h"
#include "ShaderCompiler.gen.h"

/** Translates preprocessed GLSL into one backend's bytecode. Implementations live in their
 *  rendering-API module and are found through reflection, so cooking can target an API without a
 *  live RenderingAPI and a packaged build that never compiles can drop them entirely. */
class ShaderCompiler : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~ShaderCompiler() { }

    virtual ShaderAPI GetAPI() const = 0;
    virtual bool SupportsPlatform(PlatformType InPlatform) const = 0;
    virtual bool Compile(const ShaderSource& InSource, CompiledShader& OutCompiled, String& OutError) = 0;

    static ShaderCompiler* Get(ShaderAPI InAPI);
    static Array<ShaderAPI> GetAPIsForPlatform(PlatformType InPlatform);
};
