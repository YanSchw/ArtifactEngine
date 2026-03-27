#include "VertexBuffer.h"
#include "RenderingAPI.h"
#include "Vertex.h"

SharedObjectPtr<VertexBuffer> VertexBuffer::Create(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) {
    return RenderingAPI::GetInstance()->CreateVertexBuffer(InVertices, InIndices);
}
