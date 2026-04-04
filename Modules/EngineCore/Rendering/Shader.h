#pragma once
#include "CoreMinimal.h"
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
    bool IsVertexFragmentShader() const { return GetShaderType() == ShaderType::VertexFragment; }
    bool IsComputeShader() const { return GetShaderType() == ShaderType::Compute; }

    static Map<String, String> PreProcess(const String& InShaderSource);

    static SharedObjectPtr<Shader> Create(const String& InShaderSource);
};