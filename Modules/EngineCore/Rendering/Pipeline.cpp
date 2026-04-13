#include "Pipeline.h"
#include "RenderingAPI.h"

#include "FrameBuffer.h"
#include "Surface.h"

SharedObjectPtr<Pipeline> Pipeline::Create(const PipelineDesc& InPipelineDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreatePipeline(InPipelineDesc);
}

void Pipeline::Bind() {
    bool isLastBoundTargetSame = false;

    auto& renderQueue = RenderingAPI::GetInstance()->GetRenderQueue();
    for (int32_t i = (int32_t)renderQueue.commands.size() - 1; i >= 0; i--) {
        const auto& cmd = renderQueue.commands[i];
        if (cmd.Type == RenderCommandType::BindPipeline) {
            const auto& data = std::get<CmdBindPipeline>(cmd.Data);
            if (data.Pipeline->GetDesc().Target.Get() == GetDesc().Target.Get()) {
                isLastBoundTargetSame = true;
            }
            break;
        }
    }

    if (!isLastBoundTargetSame) {
        RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::BeginRenderPass, CmdBeginRenderPass{ GetDesc().Target });
    }

    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::BindPipeline, CmdBindPipeline{ this });
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
