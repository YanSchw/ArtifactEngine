#pragma once
#include "ScreenEffectPass.h"
#include "FilmGrainPass.gen.h"

/** Lays animated film grain over the final image. */
class FilmGrainPass : public ScreenEffectPass {
public:
    ARTIFACT_CLASS();

    virtual int32_t GetOrderPreferenceIndex() const override { return 300; }

protected:
    virtual const char* GetShaderPath() const override { return "/Shaders/PostProcess/FilmGrain.glsl"; }
    virtual bool IsActive(const RenderContext& InContext) const override;
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) override;

private:
    double m_Time = 0.0;
};
