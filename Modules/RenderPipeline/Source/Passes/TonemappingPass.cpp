#include "TonemappingPass.h"

struct TonemappingUniforms {
    Vec4 Params = Vec4(0.0f);
};

bool TonemappingPass::IsActive(const RenderContext& InContext) const {
    return InContext.PostProcess.Tonemapping.Mode != TonemappingMode::None && HasShader();
}

void TonemappingPass::Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    TonemappingUniforms uniforms;
    const TonemappingParams& params = InContext.PostProcess.Tonemapping;
    uniforms.Params = Vec4(glm::exp2(params.Exposure), (float)params.Mode, glm::max(params.WhitePoint, 1e-3f), 0.0f);
    DrawEffect(InSource, InTarget, uniforms);
}
