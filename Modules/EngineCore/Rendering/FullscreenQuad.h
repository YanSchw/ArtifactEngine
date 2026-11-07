#pragma once
#include "CoreMinimal.h"
#include "Pipeline.h"

class VertexBuffer;

/** Covers the whole target, rebuilding its pipeline whenever the target or a binding changes. */
class FullscreenQuad {
public:
    FullscreenQuad();
    ~FullscreenQuad();

    void Draw(const PipelineDesc& InDesc);

private:
    static Array<void*> Identify(const PipelineDesc& InDesc);

    SharedObjectPtr<VertexBuffer> m_Quad;
    SharedObjectPtr<Pipeline> m_Pipeline;
    Array<void*> m_Identity;
};
