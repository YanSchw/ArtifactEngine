#include "Shader.h"
#include "RenderingAPI.h"

SharedObjectPtr<Shader> Shader::Create(const CompiledShader& InCompiledShader) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");

    if (!InCompiledShader.IsValid()) {
        AE_ERROR("Cannot create a shader from an invalid CompiledShader");
        return nullptr;
    }

    if (InCompiledShader.API != RenderingAPI::GetInstance()->GetShaderAPI()) {
        AE_ERROR("CompiledShader was built for a different rendering API");
        return nullptr;
    }

    if (InCompiledShader.HasStage(ShaderStage::Compute) && !RenderingAPI::GetInstance()->GetCapabilities().ComputeShaders) {
        AE_ERROR("The active rendering API does not support compute shaders");
        return nullptr;
    }

    return RenderingAPI::GetInstance()->CreateShader(InCompiledShader);
}
