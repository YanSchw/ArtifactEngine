#pragma once
#include "Core/Object.h"
#include "Core/Pointer.h"
#include "VertexBuffer.gen.h"

class VertexBuffer : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~VertexBuffer() { }
    virtual uint32_t GetIndexCount() const = 0;

    void Bind();
    void Draw();

    static SharedObjectPtr<VertexBuffer> Create(const Array<struct Vertex>& InVertices, const Array<uint32_t>& InIndices);
};