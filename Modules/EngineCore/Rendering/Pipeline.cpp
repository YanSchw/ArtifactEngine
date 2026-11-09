#include "Pipeline.h"
#include "RenderingAPI.h"

#include "FrameBuffer.h"
#include "Surface.h"

SharedObjectPtr<Pipeline> Pipeline::Create(const PipelineDesc& InPipelineDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreatePipeline(InPipelineDesc);
}

void Pipeline::Bind() {
    RenderCommandQueue& queue = RenderingAPI::GetInstance()->GetRenderQueue();
    if (queue.GetOpenRenderTarget() != m_Desc.Target.Get()) {
        queue.Push(RenderCommandType::BeginRenderPass, CmdBeginRenderPass{ m_Desc.Target });
    }
    queue.Push(RenderCommandType::BindPipeline, CmdBindPipeline{ this });
}

void Pipeline::InvalidateAll() {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    RenderingAPI::GetInstance()->InvalidateAllPipelines();
}

bool PipelineDesc::IsFrameBufferTarget() const {
    return Target && Target->IsA<FrameBuffer>();
}

bool PipelineDesc::IsSurfaceTarget() const {
    return Target && Target->IsA<Surface>();
}
