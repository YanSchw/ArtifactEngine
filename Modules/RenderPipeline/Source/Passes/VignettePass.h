#pragma once
#include "ScreenEffectPass.h"
#include "VignettePass.gen.h"

/** Darkens the image towards the screen's edges. */
class VignettePass : public ScreenEffectPass {
public:
    ARTIFACT_CLASS();

    virtual int32_t GetOrderPreferenceIndex() const override { return 200; }

protected:
    virtual const char* GetShaderPath() const override { return "/Shaders/PostProcess/Vignette.glsl"; }
    virtual bool IsActive(const RenderContext& InContext) const override;
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) override;
};