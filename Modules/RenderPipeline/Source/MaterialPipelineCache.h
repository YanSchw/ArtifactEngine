#pragma once
#include "CoreMinimal.h"
#include "Common/UUID.h"
#include "Rendering/Vertex.h"

class FrameBuffer;
class Material;
class Pipeline;
class Sampler;
struct RenderContext;

/** One pipeline per material, rebuilt whenever the target or a resource it was created from is replaced. */
class MaterialPipelineCache {
public:
    MaterialPipelineCache(const Array<ShaderDataType>& InVertexLayout = Vertex::GetLayout());
    ~MaterialPipelineCache();

    Pipeline* Resolve(Material& InMaterial, FrameBuffer& InTarget, const RenderContext& InContext);

private:
    struct Entry {
        SharedObjectPtr<Pipeline> Instance;
        Array<void*> Resources;
    };

    Map<UUID, Entry> m_Entries;
    SharedObjectPtr<Sampler> m_Sampler;
    Array<ShaderDataType> m_VertexLayout;
};
