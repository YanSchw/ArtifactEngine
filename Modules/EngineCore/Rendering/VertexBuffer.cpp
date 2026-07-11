#include "VertexBuffer.h"
#include "RenderingAPI.h"
#include "Vertex.h"
#include "CoreMinimal.h"

SharedObjectPtr<VertexBuffer> VertexBuffer::Create(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) {
    return Create(InVertices.IsEmpty() ? nullptr : &InVertices[0], (uint32_t)(InVertices.Size() * sizeof(Vertex)), InIndices);
}

SharedObjectPtr<VertexBuffer> VertexBuffer::Create(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateVertexBuffer(InVertexData, InVertexByteSize, InIndices);
}

void VertexBuffer::Bind() {
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::BindVertexBuffer, CmdBindVertexBuffer{ this });
}

void VertexBuffer::Draw() {
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::BindVertexBuffer, CmdBindVertexBuffer{ this });
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::DrawIndexed, CmdDrawIndexed{ GetIndexCount(), 0, 0 });
}

void VertexBuffer::Draw(uint32_t InIndexCount, uint32_t InFirstIndex) {
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::BindVertexBuffer, CmdBindVertexBuffer{ this });
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::DrawIndexed, CmdDrawIndexed{ InIndexCount, InFirstIndex, 0 });
}