#pragma once
#include "CoreMinimal.h"
#include "Rendering/Vertex.h"
#include "RenderingAPI.gen.h"

class RenderingAPI : public Object {
public:
    ARTIFACT_CLASS();

    RenderingAPI();
    static RenderingAPI* GetInstance() { return s_Instance; }

    virtual void Initialize() = 0;
    virtual void UpdateUniformData() = 0;
    virtual void Draw() = 0;
    virtual void CleanUp(bool InShouldDestroy) = 0;

    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) = 0;

private:
    inline static RenderingAPI* s_Instance = nullptr;
};