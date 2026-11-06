#include "ComputePipeline.h"
#include "RenderingAPI.h"
#include "Shader.h"

SharedObjectPtr<ComputePipeline> ComputePipeline::Create(const ComputePipelineDesc& InComputePipelineDesc) {
    RenderingAPI* renderingAPI = RenderingAPI::GetInstance();
    AE_ASSERT(renderingAPI, "No rendering API instance found!");

    if (!renderingAPI->GetCapabilities().ComputeShaders) {
        AE_ERROR("The active rendering API does not support compute shaders");
        return nullptr;
    }
    if (!InComputePipelineDesc.Shader || !InComputePipelineDesc.Shader->IsComputeShader()) {
        AE_ERROR("A compute pipeline needs a compute shader");
        return nullptr;
    }

    return renderingAPI->CreateComputePipeline(InComputePipelineDesc);
}

void ComputePipeline::Bind() {
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::BindComputePipeline, CmdBindComputePipeline{ this });
}

void ComputePipeline::Dispatch(uint32_t InGroupCountX, uint32_t InGroupCountY, uint32_t InGroupCountZ) {
    if (InGroupCountX == 0 || InGroupCountY == 0 || InGroupCountZ == 0) {
        return;
    }

    const RenderingCapabilities& capabilities = RenderingAPI::GetInstance()->GetCapabilities();
    if (InGroupCountX > capabilities.MaxComputeWorkGroupCount[0] ||
        InGroupCountY > capabilities.MaxComputeWorkGroupCount[1] ||
        InGroupCountZ > capabilities.MaxComputeWorkGroupCount[2]) {
        AE_ERROR("Dispatch of {0}x{1}x{2} work groups exceeds the device limit of {3}x{4}x{5}",
                 InGroupCountX, InGroupCountY, InGroupCountZ,
                 capabilities.MaxComputeWorkGroupCount[0], capabilities.MaxComputeWorkGroupCount[1], capabilities.MaxComputeWorkGroupCount[2]);
        return;
    }

    RenderCommandQueue& queue = RenderingAPI::GetInstance()->GetRenderQueue();
    if (queue.GetBoundComputePipeline() != this) {
        Bind();
    }
    queue.Push(RenderCommandType::Dispatch, CmdDispatch{ InGroupCountX, InGroupCountY, InGroupCountZ });
}

void ComputePipeline::Execute(uint32_t InGroupCountX, uint32_t InGroupCountY, uint32_t InGroupCountZ) {
    Bind();
    Dispatch(InGroupCountX, InGroupCountY, InGroupCountZ);
}