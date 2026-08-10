#pragma once
#include "Rendering/ShaderCompiler.h"
#include "WebGLShaderCompiler.gen.h"

/** Cooks the GLSL ES 3.00 a WebGL 2 build loads. The translation is pure text, so it runs on the
 *  host that cooks the package rather than in the browser. */
class WebGLShaderCompiler : public ShaderCompiler {
public:
    ARTIFACT_CLASS();

    virtual ShaderAPI GetAPI() const override { return ShaderAPI::WebGL; }
    virtual bool SupportsPlatform(PlatformType InPlatform) const override { return InPlatform == PlatformType::Web; }
    virtual bool Compile(const ShaderSource& InSource, CompiledShader& OutCompiled, String& OutError) override;
};
