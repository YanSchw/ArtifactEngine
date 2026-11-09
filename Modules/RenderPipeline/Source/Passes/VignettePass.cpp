#include "VignettePass.h"

/** Mirrors VignetteBlock in /Shaders/PostProcess/Vignette.glsl. */
struct VignetteUniforms {
    Vec4 Params = Vec4(0.0f);
    Vec4 Center = Vec4(0.0f);
    Vec4 Tint = Vec4(0.0f);
};

bool VignettePass::IsActive(const RenderContext& InContext) const {
    return InContext.PostProcess.Vignette.Intensity > 0.0f && HasShader();
}

void VignettePass::Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    const VignetteParams& params = InContext.PostProcess.Vignette;
    VignetteUniforms uniforms;
    uniforms.Params = Vec4(params.Intensity * 3.0f, glm::clamp(params.Smoothness, 0.01f, 1.0f) * 5.0f,
                           glm::clamp(params.Roundness, 0.0f, 1.0f), (float)InContext.Width / (float)InContext.Height);
    uniforms.Center = Vec4(params.Center, 0.0f, 0.0f);
    uniforms.Tint = params.Tint;
    DrawEffect(InSource, InTarget, uniforms);
}
