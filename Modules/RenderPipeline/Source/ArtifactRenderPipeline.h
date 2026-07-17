#pragma once
#include "Rendering/RenderPipeline.h"
#include "Object/Pointer.h"
#include "ArtifactRenderPipeline.gen.h"

class UniformBuffer;
class Pipeline;
class FrameBuffer;

class ArtifactRenderPipeline : public RenderPipeline {
public:
    ARTIFACT_CLASS();

    ArtifactRenderPipeline();
    ~ArtifactRenderPipeline();

    void Invalidate(uint32_t InWidth, uint32_t InHeight);

    virtual void Render(double InDeltaTime, const RenderParams& InParams) override;
    virtual SharedObjectPtr<class ImageView> GetFinalImageView() const override;

private:
    void UpdateUniformData(const RenderParams& InParams);

    SharedObjectPtr<UniformBuffer> m_UniformBuffer;
    SharedObjectPtr<Pipeline> m_Pipeline;
    SharedObjectPtr<FrameBuffer> m_FrameBuffer;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};
