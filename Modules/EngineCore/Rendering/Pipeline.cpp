#include "Pipeline.h"
#include "RenderingAPI.h"

SharedObjectPtr<Pipeline> Pipeline::Create(const PipelineDesc& InPipelineDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreatePipeline(InPipelineDesc);
}

void Pipeline::Bind() {
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::BindPipeline, CmdBindPipeline{ this });
}

void Pipeline::InvalidateAll() {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    RenderingAPI::GetInstance()->InvalidateAllPipelines();
}
