#pragma once
#include "CoreMinimal.h"
#include "Vertex.h"
#include "Pipeline.gen.h"

#include <variant>

struct PipelineDesc {
    SharedObjectPtr<class Shader> Shader;
    Array<ShaderDataType> VertexLayout = Vertex::GetLayout();
    Array<SharedObjectPtr<class ShaderBuffer>> Buffers;
    Array<std::tuple<uint32_t, SharedObjectPtr<class ImageView>, SharedObjectPtr<class Sampler>>> ImageBindings;

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
    virtual PipelineDesc GetDesc() const = 0;

    void Bind();
    static void InvalidateAll();
    static SharedObjectPtr<Pipeline> Create(const PipelineDesc& InPipelineDesc);
};