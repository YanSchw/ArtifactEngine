#pragma once
#include "Rendering/PostProcessPass.h"
#include "Rendering/FullscreenQuad.h"
#include "AntiAliasingPass.gen.h"

class Sampler;
class Shader;
class UniformBuffer;

/** Smooths edges with FXAA or TAA. */
class AntiAliasingPass : public PostProcessPass {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Initialize() override;
    virtual bool IsActive(const RenderContext& InContext) const override;
    virtual void PrepareFrame(RenderContext& InOutContext) override;
    virtual FrameBuffer& AcquireTarget(const RenderContext& InContext, PostProcessChain& InChain) override;
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) override;

private:
    void ProcessFXAA(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget);
    void ProcessTAA(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget);
    /** Returns true when the history targets had to be (re)created. */
    bool EnsureHistory(uint32_t InWidth, uint32_t InHeight);

    SharedObjectPtr<Shader> m_FxaaShader;
    SharedObjectPtr<Shader> m_TaaShader;
    SharedObjectPtr<Sampler> m_LinearSampler;
    SharedObjectPtr<Sampler> m_PointSampler;
    SharedObjectPtr<UniformBuffer> m_FxaaUniforms;
    SharedObjectPtr<UniformBuffer> m_TaaUniforms;
    FullscreenQuad m_FxaaQuad;

    /** Each frame draws into one history target while reading the other. */
    SharedObjectPtr<FrameBuffer> m_History[2];
    FullscreenQuad m_TaaQuads[2];
    int32_t m_HistoryWrite = 0;
    bool m_HasHistory = false;
    bool m_HistoryValid = false;
    uint64_t m_HistoryFrame = 0;
    Mat4 m_PreviousViewProjection = Mat4(1.0f);
};
