#include "ChromaticAberrationPass.h"

/** Mirrors ChromaticAberrationBlock in /Shaders/PostProcess/ChromaticAberration.glsl. */
struct ChromaticAberrationUniforms {
    Vec4 Params = Vec4(0.0f);
    Vec4 Screen = Vec4(0.0f);
};

bool ChromaticAberrationPass::IsActive(const RenderContext& InContext) const {
    return InContext.PostProcess.ChromaticAberration.Intensity > 0.0f && HasShader();
}

void ChromaticAberrationPass::Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    const ChromaticAberrationParams& params = InContext.PostProcess.ChromaticAberration;
    ChromaticAberrationUniforms uniforms;
    uniforms.Params = Vec4(params.Intensity * 0.01f, glm::clamp(params.StartRadius, 0.0f, 0.99f),
                           glm::max(params.Falloff, 0.01f), (float)glm::clamp(params.SampleCount, 3, 16));
    uniforms.Screen = Vec4((float)InContext.Width / (float)InContext.Height, 0.0f, 0.0f, 0.0f);
    DrawEffect(InSource, InTarget, uniforms);
}