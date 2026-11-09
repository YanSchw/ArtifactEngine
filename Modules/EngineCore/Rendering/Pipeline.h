#pragma once
#include "CoreMinimal.h"
#include "Vertex.h"
#include "PipelineResources.h"

#include "Pipeline.gen.h"

struct PipelineDesc : public PipelineResources {
    Array<ShaderDataType> VertexLayout = Vertex::GetLayout();
    /** Per-instance attributes. */
    Array<ShaderDataType> InstanceLayout;

    /* Target has to be of class Surface or FrameBuffer */
    SharedObjectPtr<Object> Target = nullptr;

    bool IsFrameBufferTarget() const;
    bool IsSurfaceTarget() const;
};

class Pipeline : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~Pipeline() { }
    virtual void Invalidate() = 0;

    const PipelineDesc& GetDesc() const { return m_Desc; }

    void Bind();

    /** Rebuilds every graphics and compute pipeline. */
    static void InvalidateAll();
    static SharedObjectPtr<Pipeline> Create(const PipelineDesc& InPipelineDesc);

protected:
    PipelineDesc m_Desc;
};
