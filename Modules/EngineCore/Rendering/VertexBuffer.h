#pragma once
#include "Core/Object.h"
#include "Core/Pointer.h"
#include "VertexBuffer.gen.h"

class VertexBuffer : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~VertexBuffer() { }
    virtual void Bind() = 0;

    static SharedObjectPtr<VertexBuffer> Create(const Array<struct Vertex>& InVertices, const Array<uint32_t>& InIndices);
};