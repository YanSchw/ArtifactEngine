#pragma once
#include "Rendering/PostProcessPass.h"
#include "Rendering/FullscreenQuad.h"
#include "AmbientOcclusionPass.gen.h"

class Sampler;
class Shader;
class UniformBuffer;

/** Screen-space ambient occlusion from the scene's depth, multiplied into the image. */
class AmbientOcclusionPass : public PostProcessPass {
public:
    ARTIFACT_CLASS();

    virtual int32_t GetOrderPreferenceIndex() const override { return -200; }

protected:
    virtual void Initialize() override;
    virtual bool IsActive(const RenderContext& InContext) const override;
    virtual void RenderIntermediates(const RenderContext& InContext, ImageView& InSource) override;
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) override;

private:
    SharedObjectPtr<Shader> m_OcclusionShader;
    SharedObjectPtr<Shader> m_BlurShader;
    SharedObjectPtr<Shader> m_CompositeShader;
    SharedObjectPtr<Sampler> m_PointSampler;
    SharedObjectPtr<UniformBuffer> m_Uniforms;

    SharedObjectPtr<FrameBuffer> m_Occlusion;
    SharedObjectPtr<FrameBuffer> m_Blurred;
    FullscreenQuad m_OcclusionQuad;
    FullscreenQuad m_BlurQuad;
    FullscreenQuad m_CompositeQuad;
    ImageView* m_Result = nullptr;
};