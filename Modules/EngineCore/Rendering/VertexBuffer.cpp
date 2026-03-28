#include "VertexBuffer.h"
#include "RenderingAPI.h"
#include "Vertex.h"
#include "CoreMinimal.h"

SharedObjectPtr<VertexBuffer> VertexBuffer::Create(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateVertexBuffer(InVertices, InIndices);
}
