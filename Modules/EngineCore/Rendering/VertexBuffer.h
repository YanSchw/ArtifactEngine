#pragma once
#include "CoreMinimal.h"
#include "VertexBuffer.gen.h"

class VertexBuffer : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~VertexBuffer() { }
    virtual uint32_t GetIndexCount() const = 0;

    /** Re-upload the contents of a dynamic buffer in place.
     *  Only valid on buffers made with CreateDynamic(); a no-op on static buffers. */
    virtual void Update(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) { (void)InVertexData; (void)InVertexByteSize; (void)InIndices; }

    void Bind();
    void Draw();
    /** Draw a sub-range of the index buffer. */
    void Draw(uint32_t InIndexCount, uint32_t InFirstIndex);

    static SharedObjectPtr<VertexBuffer> Create(const Array<struct Vertex>& InVertices, const Array<uint32_t>& InIndices);
    static SharedObjectPtr<VertexBuffer> Create(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices);
    /** Creates an empty host-visible buffer intended to be re-uploaded every frame via Update().
     *  Use this for per-frame geometry (UI, debug draws) instead of Create(), which stages into
     *  device-local memory and stalls the GPU queue on every call. */
    static SharedObjectPtr<VertexBuffer> CreateDynamic();
};