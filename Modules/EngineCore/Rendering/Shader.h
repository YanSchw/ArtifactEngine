#pragma once
#include "CoreMinimal.h"
#include "CompiledShader.h"
#include "Shader.gen.h"

enum class ShaderType {
    Unkown = 0,
    VertexFragment,
    Compute
};

class Shader : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~Shader() { }
    virtual ShaderType GetShaderType() const = 0;
    virtual void Reload(const CompiledShader& InCompiledShader) = 0;

    bool IsVertexFragmentShader() const { return GetShaderType() == ShaderType::VertexFragment; }
    bool IsComputeShader() const { return GetShaderType() == ShaderType::Compute; }

    const ShaderRenderState& GetRenderState() const { return m_RenderState; }

    static SharedObjectPtr<Shader> Create(const CompiledShader& InCompiledShader);

protected:
    ShaderRenderState m_RenderState;
};
