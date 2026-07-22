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

    static constexpr int32_t NodeIdAttachment = 1;

    ArtifactRenderPipeline();
    ~ArtifactRenderPipeline();

    void Invalidate(uint32_t InWidth, uint32_t InHeight);

    virtual void Render(double InDeltaTime, const RenderParams& InParams) override;
    virtual SharedObjectPtr<class ImageView> GetFinalImageView() const override;
    virtual SharedObjectPtr<class FrameBuffer> GetFrameBuffer() const override { return m_FrameBuffer; }
    virtual uint32_t PickNodeId(uint32_t InX, uint32_t InY) const override;

private:
    void UpdateUniformData(const RenderParams& InParams);

    SharedObjectPtr<UniformBuffer> m_UniformBuffer;
    SharedObjectPtr<Pipeline> m_Pipeline;
    SharedObjectPtr<FrameBuffer> m_FrameBuffer;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};
