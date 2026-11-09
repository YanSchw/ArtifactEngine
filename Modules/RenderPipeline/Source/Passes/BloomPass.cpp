#include "BloomPass.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"

static constexpr uint32_t s_SourceBinding = 16;
static constexpr uint32_t s_SecondBinding = 17;

/** Mirrors BloomBlock in /Shaders/PostProcess/BloomCommon.glsl. */
struct BloomUniforms {
    Vec4 Params = Vec4(0.0f);
    Vec4 Tint = Vec4(0.0f);
};

static ImageView* ColorOf(const SharedObjectPtr<FrameBuffer>& InTarget) {
    return InTarget->GetDesc().ColorAttachments[0].Get();
}

void BloomPass::Initialize() {
    m_PrefilterShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/BloomPrefilter.glsl");
    m_DownsampleShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/BloomDownsample.glsl");
    m_UpsampleShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/BloomUpsample.glsl");
    m_CompositeShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/BloomComposite.glsl");
    m_LinearSampler = Sampler::Create({ FilterMode::Linear, FilterMode::Linear,
                                        AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
    m_Uniforms = UniformBuffer::Create(1, sizeof(BloomUniforms));
}

bool BloomPass::IsActive(const RenderContext& InContext) const {
    return InContext.PostProcess.Bloom.Intensity > 0.0f
        && m_PrefilterShader && m_DownsampleShader && m_UpsampleShader && m_CompositeShader;
}

void BloomPass::RenderIntermediates(const RenderContext& InContext, ImageView& InSource) {
    const BloomParams& params = InContext.PostProcess.Bloom;
    const float threshold = glm::max(params.Threshold, 0.0f);

    BloomUniforms uniforms;
    uniforms.Params = Vec4(threshold, threshold * glm::clamp(params.SoftKnee, 0.0f, 1.0f),
                           glm::max(params.MaxBrightness, threshold + 1e-3f), glm::clamp(params.Scatter, 0.0f, 1.0f));
    uniforms.Tint = Vec4(Vec3(params.Tint) * glm::max(params.Intensity, 0.0f), 1.0f);
    Upload(*m_Uniforms, uniforms);

    // Halving rounds up, as the downsample shaders expect, and stops before a mip gets thinner than two texels.
    uint32_t width = InContext.Width;
    uint32_t height = InContext.Height;
    int32_t mipCount = 0;
    const int32_t wantedMips = glm::clamp(params.MipCount, 1, MaxMipCount);
    while (mipCount < wantedMips && (mipCount == 0 || (width > 2 && height > 2))) {
        width = glm::max((width + 1) / 2, 1u);
        height = glm::max((height + 1) / 2, 1u);
        EnsureTarget(m_Down[mipCount], width, height, ChainFormat);
        mipCount++;
    }

    ImageView* source = &InSource;
    for (int32_t mip = 0; mip < mipCount; mip++) {
        m_Down[mip]->Begin();
        PipelineDesc desc;
        desc.Target = m_Down[mip];
        desc.Shader = mip == 0 ? m_PrefilterShader : m_DownsampleShader;
        desc.Buffers.Add(m_Uniforms);
        desc.ImageBindings.Add({ s_SourceBinding, source, m_LinearSampler });
        m_DownQuads[mip].Draw(desc);
        source = ColorOf(m_Down[mip]);
    }

    ImageView* coarse = ColorOf(m_Down[mipCount - 1]);
    for (int32_t mip = mipCount - 2; mip >= 0; mip--) {
        const FrameBufferDesc& detail = m_Down[mip]->GetDesc();
        EnsureTarget(m_Up[mip], detail.Width, detail.Height, ChainFormat);
        m_Up[mip]->Begin();
        PipelineDesc desc;
        desc.Target = m_Up[mip];
        desc.Shader = m_UpsampleShader;
        desc.Buffers.Add(m_Uniforms);
        desc.ImageBindings.Add({ s_SourceBinding, coarse, m_LinearSampler });
        desc.ImageBindings.Add({ s_SecondBinding, detail.ColorAttachments[0], m_LinearSampler });
        m_UpQuads[mip].Draw(desc);
        coarse = ColorOf(m_Up[mip]);
    }
    m_Result = coarse;
}

void BloomPass::Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    (void)InContext;
    PipelineDesc desc;
    desc.Target = &InTarget;
    desc.Shader = m_CompositeShader;
    desc.Buffers.Add(m_Uniforms);
    desc.ImageBindings.Add({ s_SourceBinding, &InSource, m_LinearSampler });
    desc.ImageBindings.Add({ s_SecondBinding, m_Result, m_LinearSampler });
    m_CompositeQuad.Draw(desc);
}
