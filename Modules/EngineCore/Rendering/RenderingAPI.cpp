#include "RenderingAPI.h"
#include "ShaderData.h"

RenderingAPI::RenderingAPI() {
    s_Instance = this;
}

void RenderingAPI::DrawIndexed(uint32_t InIndexCount, uint32_t InFirstIndex, int32_t InVertexOffset) {
    GetRenderQueue().Push(RenderCommandType::DrawIndexed, CmdDrawIndexed{ InIndexCount, InFirstIndex, InVertexOffset });
}

void RenderingAPI::SetShaderData(ShaderData* InShaderData) {
    GetRenderQueue().Push(RenderCommandType::SetShaderData, CmdSetShaderData{ InShaderData });
}

void RenderingAPI::WriteTimestamp(uint32_t InQuery) {
    if (InQuery != InvalidTimestampQuery) {
        GetRenderQueue().Push(RenderCommandType::WriteTimestamp, CmdWriteTimestamp{ InQuery });
    }
}

bool RenderingCapabilities::Supports(RenderCapability InCapabilities) const {
    auto needs = [InCapabilities](RenderCapability InCapability) {
        return (InCapabilities & InCapability) != RenderCapability::None;
    };
    return (!needs(RenderCapability::Compute) || ComputeShaders)
        && (!needs(RenderCapability::StorageBuffers) || StorageBuffers)
        && (!needs(RenderCapability::StorageImages) || StorageImages)
        && (!needs(RenderCapability::Timestamps) || Timestamps);
}
