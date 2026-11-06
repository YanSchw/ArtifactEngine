#include "RenderingCommand.h"
#include "ComputePipeline.h"

RenderCommandQueue::RenderCommandQueue() {
    commands.reserve(1024);
}

void RenderCommandQueue::Clear() {
    commands.clear();
    commands.reserve(1024);
    m_OpenRenderTarget = nullptr;
    m_BoundComputePipeline = nullptr;
}

void RenderCommandQueue::Track(const RenderCommand& InCommand) {
    switch (InCommand.Type) {
        case RenderCommandType::BeginRenderPass:
            m_OpenRenderTarget = std::get<CmdBeginRenderPass>(InCommand.Data).Target;
            m_BoundComputePipeline = nullptr;
            break;
        case RenderCommandType::BindPipeline:
            m_BoundComputePipeline = nullptr;
            break;
        case RenderCommandType::BindComputePipeline:
            m_OpenRenderTarget = nullptr;
            m_BoundComputePipeline = std::get<CmdBindComputePipeline>(InCommand.Data).Pipeline;
            break;
        default:
            break;
    }
}
