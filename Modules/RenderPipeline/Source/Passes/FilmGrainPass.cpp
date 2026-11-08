#include "FilmGrainPass.h"

/** Mirrors FilmGrainBlock in /Shaders/PostProcess/FilmGrain.glsl. */
struct FilmGrainUniforms {
    Vec4 Params = Vec4(0.0f);
    Vec4 Seed = Vec4(0.0f);
};

bool FilmGrainPass::IsActive(const RenderContext& InContext) const {
    return InContext.PostProcess.FilmGrain.Intensity > 0.0f && HasShader();
}

void FilmGrainPass::Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) {
    const FilmGrainParams& params = InContext.PostProcess.FilmGrain;
    m_Time += InContext.DeltaTime;

    uint64_t frame = 0;
    if (params.Animated) {
        frame = params.FramesPerSecond > 0.0f ? (uint64_t)(m_Time * params.FramesPerSecond) : InContext.FrameIndex;
    }
    // Wrapped so the offsets stay exact in floats.
    const float seed = (float)(frame % 4096);

    FilmGrainUniforms uniforms;
    uniforms.Params = Vec4(glm::max(params.Intensity, 0.0f),
                           glm::max(glm::max(params.Size, 0.1f) * (float)InContext.Height / 1080.0f, 1.0f),
                           glm::clamp(params.LuminanceResponse, 0.0f, 1.0f), glm::clamp(params.Colorfulness, 0.0f, 1.0f));
    uniforms.Seed = Vec4(seed, 0.0f, 0.0f, 0.0f);
    DrawEffect(InSource, InTarget, uniforms);
}