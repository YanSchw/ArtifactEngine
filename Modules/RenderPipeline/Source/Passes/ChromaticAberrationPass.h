#pragma once
#include "ScreenEffectPass.h"
#include "ChromaticAberrationPass.gen.h"

/** Splits the colours apart towards the screen's edges. */
class ChromaticAberrationPass : public ScreenEffectPass {
public:
    ARTIFACT_CLASS();

    virtual int32_t GetOrderPreferenceIndex() const override { return 100; }

protected:
    virtual const char* GetShaderPath() const override { return "/Shaders/PostProcess/ChromaticAberration.glsl"; }
    virtual bool IsActive(const RenderContext& InContext) const override;
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) override;
};
