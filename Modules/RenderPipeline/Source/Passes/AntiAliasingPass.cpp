#include "AntiAliasingPass.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"
#include "GameFramework/CameraNode.h"

static constexpr uint32_t s_SourceBinding = 16;
static constexpr uint32_t s_HistoryBinding = 17;
static constexpr uint32_t s_DepthBinding = 18;
static constexpr uint32_t s_JitterSequenceLength = 8;

/** Mirrors FxaaBlock in /Shaders/PostProcess/FXAA.glsl. */
struct FxaaUniforms {
    Vec4 Params = Vec4(0.0f);
};

/** Mirrors TaaBlock in /Shaders/PostProcess/TAA.glsl. */
struct TaaUniforms {
    Mat4 InverseViewProjection = Mat4(1.0f);
    Mat4 PreviousViewProjection = Mat4(1.0f);
    Vec4 Params = Vec4(0.0f);
};

static float Halton(uint32_t InIndex, uint32_t InBase) {
    float result = 0.0f;
    float fraction = 1.0f;
    while (InIndex > 0) {
        fraction /= (float)InBase;
        result += fraction * (float)(InIndex % InBase);
        InIndex /= InBase;
    }
    return result;
}

void AntiAliasingPass::Initialize() {
    m_FxaaShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/FXAA.glsl");
    m_TaaShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/TAA.glsl");
    m_LinearSampler = Sampler::Create({ FilterMode::Linear, FilterMode::Linear,
                                        AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
    m_PointSampler = Sampler::Create({ FilterMode::Nearest, FilterMode::Nearest,
                                       AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
    m_FxaaUniforms = UniformBuffer::Create(1, sizeof(FxaaUniforms));
    m_TaaUniforms = UniformBuffer::Create(1, sizeof(TaaUniforms));
}

bool AntiAliasingPass::IsActive(const RenderContext& InContext) const {
    switch (InContext.PostProcess.AntiAliasing.Mode) {
        case AntiAliasingMode::FXAA: return m_FxaaShader.Get() != nullptr;
        case AntiAliasingMode::TAA: return m_TaaShader.Get() != nullptr && InContext.Camera != nullptr;
        default: return false;
    }
}

void AntiAliasingPass::PrepareFrame(RenderContext& InOutContext) {
    if (!IsRunning(InOutContext) || InOutContext.PostProcess.AntiAliasing.Mode != AntiAliasingMode::TAA) {
        return;
    }
    const uint32_t sample = (uint32_t)(InOutContext.FrameIndex % s_JitterSequenceLength) + 1;
    const Vec2 offset(Halton(sample, 2) - 0.5f, Halton(sample, 3) - 0.5f);
    InOutContext.ProjectionJitter = offset * 2.0f / Vec2((float)InOutContext.Width, (float)InOutContext.Height);
}

bool AntiAliasingPass::EnsureHistory(uint32_t InWidth, uint32_t InHeight) {
    if (m_History[0] && m_History[0]->GetDesc().Width == InWidth && m_History[0]->GetDesc().Height == InHeight) {
        return false;
    }
    if (m_History[0]) {
        RenderingAPI::GetInstance()->WaitIdle();
    }
    for (SharedObjectPtr<FrameBuffer>& history : m_History) {
        FrameBufferDesc desc;
        desc.Width = InWidth;
        desc.Height = InHeight;
        desc.ColorAttachments.Add(ImageView::CreateAttachment(InWidth, InHeight, ImageFormat::RGBA16F,
                                                              ImageUsage::ColorAttachment | ImageUsage::Sampled));
        desc.ClearColor = Vec4(0.0f);
        history = FrameBuffer::Create(desc);
        // Cleared once, so the target read before anything was drawn into it is in a readable layout.
        history->Begin();
    }
    return true;
}

FrameBuffer& AntiAliasingPass::AcquireTarget(const RenderContext& InContext, PostProcessChain& InChain) {
    if (InContext.PostProcess.AntiAliasing.Mode != AntiAliasingMode::TAA) {
        m_HasHistory = false;
        return PostProcessPass::AcquireTarget(InContext, InChain);
    }
    const bool recreated = EnsureHistory(InContext.Width, InContext.Height);
    m_HistoryValid = m_HasHistory && !recreated && m_HistoryFrame + 1 == InContext.FrameIndex;
    return *m_History[m_HistoryWrite];
}

void AntiAliasingPass::Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    if (InContext.PostProcess.AntiAliasing.Mode == AntiAliasingMode::TAA) {
        ProcessTAA(InContext, InSource, InTarget);
    } else {
        ProcessFXAA(InContext, InSource, InTarget);
    }
}

void AntiAliasingPass::ProcessFXAA(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    const AntiAliasingParams& params = InContext.PostProcess.AntiAliasing;
    FxaaUniforms uniforms;
    uniforms.Params = Vec4(glm::max(params.FxaaEdgeThreshold, 0.0f), glm::max(params.FxaaEdgeThresholdMin, 0.0f),
                           glm::clamp(params.FxaaSubpixelBlend, 0.0f, 1.0f), 0.0f);
    Upload(*m_FxaaUniforms, uniforms);

    PipelineDesc desc;
    desc.Target = &InTarget;
    desc.Shader = m_FxaaShader;
    desc.Buffers.Add(m_FxaaUniforms);
    desc.ImageBindings.Add({ s_SourceBinding, &InSource, m_LinearSampler });
    m_FxaaQuad.Draw(desc);
}

void AntiAliasingPass::ProcessTAA(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    const Mat4 viewProjection = InContext.Camera->GetViewProjectionMatrix();

    TaaUniforms uniforms;
    uniforms.InverseViewProjection = glm::inverse(viewProjection);
    uniforms.PreviousViewProjection = m_HistoryValid ? m_PreviousViewProjection : viewProjection;
    uniforms.Params = Vec4(glm::clamp(InContext.PostProcess.AntiAliasing.TaaFeedback, 0.0f, 0.98f),
                           m_HistoryValid ? 1.0f : 0.0f,
                           InContext.ProjectionJitter * 0.5f);
    Upload(*m_TaaUniforms, uniforms);

    PipelineDesc desc;
    desc.Target = &InTarget;
    desc.Shader = m_TaaShader;
    desc.Buffers.Add(m_TaaUniforms);
    desc.ImageBindings.Add({ s_SourceBinding, &InSource, m_LinearSampler });
    desc.ImageBindings.Add({ s_HistoryBinding, m_History[1 - m_HistoryWrite]->GetDesc().ColorAttachments[0], m_LinearSampler });
    desc.ImageBindings.Add({ s_DepthBinding, InContext.PostChain->GetScene().GetDesc().DepthAttachment, m_PointSampler });
    m_TaaQuads[m_HistoryWrite].Draw(desc);

    m_PreviousViewProjection = viewProjection;
    m_HistoryFrame = InContext.FrameIndex;
    m_HasHistory = true;
    m_HistoryWrite = 1 - m_HistoryWrite;
}
