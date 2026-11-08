#pragma once
#include "Rendering/PostProcessPass.h"
#include "Rendering/FullscreenQuad.h"
#include "BloomPass.gen.h"

class Sampler;
class Shader;
class UniformBuffer;

/** Blurs what is brighter than the threshold through a mip chain and adds it back onto the image. */
class BloomPass : public PostProcessPass {
public:
    ARTIFACT_CLASS();

    static constexpr int32_t MaxMipCount = 8;

    virtual int32_t GetOrderPreferenceIndex() const override { return -100; }

protected:
    virtual void Initialize() override;
    virtual bool IsActive(const RenderContext& InContext) const override;
    virtual void RenderIntermediates(const RenderContext& InContext, ImageView& InSource) override;
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) override;

private:
    SharedObjectPtr<Shader> m_PrefilterShader;
    SharedObjectPtr<Shader> m_DownsampleShader;
    SharedObjectPtr<Shader> m_UpsampleShader;
    SharedObjectPtr<Shader> m_CompositeShader;
    SharedObjectPtr<Sampler> m_LinearSampler;
    SharedObjectPtr<UniformBuffer> m_Uniforms;

    SharedObjectPtr<FrameBuffer> m_Down[MaxMipCount];
    SharedObjectPtr<FrameBuffer> m_Up[MaxMipCount - 1];
    FullscreenQuad m_DownQuads[MaxMipCount];
    FullscreenQuad m_UpQuads[MaxMipCount - 1];
    FullscreenQuad m_CompositeQuad;
    ImageView* m_Result = nullptr;
};