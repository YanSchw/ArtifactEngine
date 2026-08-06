#pragma once
#include "Rendering/ShaderCompiler.h"
#include "VulkanShaderCompiler.gen.h"

class VulkanShaderCompiler : public ShaderCompiler {
public:
    ARTIFACT_CLASS();

    virtual ShaderAPI GetAPI() const override { return ShaderAPI::Vulkan; }
    virtual bool SupportsPlatform(PlatformType InPlatform) const override;
    virtual bool Compile(const ShaderSource& InSource, CompiledShader& OutCompiled, String& OutError) override;
};
