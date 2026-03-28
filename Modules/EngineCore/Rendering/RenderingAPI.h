#pragma once
#include "CoreMinimal.h"
#include "Rendering/Vertex.h"
#include "Rendering/RenderingCommand.h"
#include "RenderingAPI.gen.h"

class RenderingAPI : public Object {
public:
    ARTIFACT_CLASS();

    RenderingAPI();
    static RenderingAPI* GetInstance() { return s_Instance; }

    void DrawIndexed(uint32_t InIndexCount, uint32_t InFirstIndex, int32_t InVertexOffset);

    virtual void Initialize() = 0;
    virtual void UpdateUniformData() = 0;
    virtual void Draw() = 0;
    virtual void CleanUp(bool InShouldDestroy) = 0;

    virtual struct RenderCommandQueue& GetRenderQueue() = 0;
    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) = 0;

private:
    inline static RenderingAPI* s_Instance = nullptr;
};