#pragma once
#include "Rendering/Shader.h"
#include "WebGLCommon.h"
#include "WebGLShader.gen.h"

class WebGLShader : public Shader {
public:
    ARTIFACT_CLASS();

    explicit WebGLShader(const CompiledShader& InCompiledShader);
    virtual ~WebGLShader();

    virtual ShaderType GetShaderType() const override { return ShaderType::VertexFragment; }
    virtual void Reload(const CompiledShader& InCompiledShader) override;

    GLuint GetProgram() const { return m_Program; }

    /** The sampler uniform declared with the given binding slot, or -1. */
    GLint GetSamplerLocation(uint32_t InBinding) const;

private:
    void Link(const CompiledShader& InCompiledShader);
    void Destroy();

    GLuint m_Program = 0;
    Map<uint32_t, GLint> m_SamplerLocations;
};
