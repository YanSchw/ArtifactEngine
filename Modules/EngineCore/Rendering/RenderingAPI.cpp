#include "RenderingAPI.h"

RenderingAPI::RenderingAPI() {
    s_Instance = this;
}

void RenderingAPI::DrawIndexed(uint32_t InIndexCount, uint32_t InFirstIndex, int32_t InVertexOffset) {
    GetRenderQueue().Push(RenderCommandType::DrawIndexed, CmdDrawIndexed{ InIndexCount, InFirstIndex, InVertexOffset });
}