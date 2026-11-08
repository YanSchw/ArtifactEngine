#include "AmbientOcclusionPass.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"
#include "GameFramework/CameraNode.h"

static constexpr uint32_t s_DepthBinding = 16;
static constexpr uint32_t s_OcclusionBinding = 17;
static constexpr uint32_t s_SourceBinding = 18;

/** Mirrors AmbientOcclusionBlock in /Shaders/PostProcess/AmbientOcclusionCommon.glsl. */
struct AmbientOcclusionUniforms {
    Mat4 InverseProjection = Mat4(1.0f);
    Vec4 Shape = Vec4(0.0f);
    Vec4 Sampling = Vec4(0.0f);
    Vec4 Fade = Vec4(0.0f);
    Vec4 Tint = Vec4(0.0f);
    Vec4 Resolution = Vec4(0.0f);
};

void AmbientOcclusionPass::Initialize() {
    m_OcclusionShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/AmbientOcclusion.glsl");
    m_BlurShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/AmbientOcclusionBlur.glsl");
    m_CompositeShader = ShaderLibrary::CreateShader("/Shaders/PostProcess/AmbientOcclusionComposite.glsl");
    m_PointSampler = Sampler::Create({ FilterMode::Nearest, FilterMode::Nearest,
                                       AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
    m_Uniforms = UniformBuffer::Create(1, sizeof(AmbientOcclusionUniforms));
}

bool AmbientOcclusionPass::IsActive(const RenderContext& InContext) const {
    return InContext.PostProcess.AmbientOcclusion.Intensity > 0.0f && InContext.Camera
        && m_OcclusionShader && m_BlurShader && m_CompositeShader;
}

void AmbientOcclusionPass::RenderIntermediates(const RenderContext& InContext, ImageView& InSource) {
    (void)InSource;
    const AmbientOcclusionParams& params = InContext.PostProcess.AmbientOcclusion;
    ImageView* depth = InContext.PostChain->GetScene().GetDesc().DepthAttachment.Get();
    const uint32_t width = params.HalfResolution ? glm::max((InContext.Width + 1) / 2, 1u) : InContext.Width;
    const uint32_t height = params.HalfResolution ? glm::max((InContext.Height + 1) / 2, 1u) : InContext.Height;

    // The scene was rendered with the jittered projection, so positions are reconstructed with it too.
    const Mat4 projection = glm::translate(Mat4(1.0f), Vec3(InContext.ProjectionJitter, 0.0f))
                          * InContext.Camera->GetProjectionMatrix();
    const bool orthographic = InContext.Camera->GetProjectionType() == CameraNode::ProjectionType::Orthographic;
    const float maxDistance = glm::max(params.MaxDistance, 0.0f);
    const bool temporal = InContext.PostProcess.AntiAliasing.Mode == AntiAliasingMode::TAA;

    AmbientOcclusionUniforms uniforms;
    uniforms.InverseProjection = glm::inverse(projection);
    uniforms.Shape = Vec4(glm::max(params.Radius, 1e-3f), glm::max(params.Bias, 0.0f),
                          glm::max(params.Intensity, 0.0f), glm::max(params.Power, 0.01f));
    uniforms.Sampling = Vec4(glm::abs(projection[1][1]) * 0.5f * (float)InContext.Height, orthographic ? 1.0f : 0.0f,
                             (float)glm::clamp(params.SampleCount, 4, 32),
                             temporal ? (float)(InContext.FrameIndex % 64) * 5.588238f : 0.0f);
    uniforms.Fade = Vec4(maxDistance * (1.0f - glm::clamp(params.DistanceFalloff, 0.0f, 1.0f)) - 1e-3f, maxDistance,
                         glm::max(params.BlurSharpness, 0.0f), glm::clamp(params.LuminanceInfluence, 0.0f, 1.0f));
    uniforms.Tint = params.Tint;
    uniforms.Resolution = Vec4((float)width, (float)height, (float)InContext.Width, (float)InContext.Height);
    Upload(*m_Uniforms, uniforms);

    EnsureTarget(m_Occlusion, width, height, ImageFormat::RGBA16F);
    m_Occlusion->Begin();
    PipelineDesc occlusion;
    occlusion.Target = m_Occlusion;
    occlusion.Shader = m_OcclusionShader;
    occlusion.Buffers.Add(m_Uniforms);
    occlusion.ImageBindings.Add({ s_DepthBinding, depth, m_PointSampler });
    m_OcclusionQuad.Draw(occlusion);
    m_Result = m_Occlusion->GetDesc().ColorAttachments[0].Get();

    if (!params.Blur) {
        return;
    }
    EnsureTarget(m_Blurred, width, height, ImageFormat::RGBA16F);
    m_Blurred->Begin();
    PipelineDesc blur;
    blur.Target = m_Blurred;
    blur.Shader = m_BlurShader;
    blur.Buffers.Add(m_Uniforms);
    blur.ImageBindings.Add({ s_DepthBinding, depth, m_PointSampler });
    blur.ImageBindings.Add({ s_OcclusionBinding, m_Result, m_PointSampler });
    m_BlurQuad.Draw(blur);
    m_Result = m_Blurred->GetDesc().ColorAttachments[0].Get();
}

void AmbientOcclusionPass::Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    PipelineDesc desc;
    desc.Target = &InTarget;
    desc.Shader = m_CompositeShader;
    desc.Buffers.Add(m_Uniforms);
    desc.ImageBindings.Add({ s_DepthBinding, InContext.PostChain->GetScene().GetDesc().DepthAttachment, m_PointSampler });
    desc.ImageBindings.Add({ s_OcclusionBinding, m_Result, m_PointSampler });
    desc.ImageBindings.Add({ s_SourceBinding, &InSource, m_PointSampler });
    m_CompositeQuad.Draw(desc);
}