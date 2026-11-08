#pragma once
#include "ScreenEffectPass.h"
#include "TonemappingPass.gen.h"

class TonemappingPass : public ScreenEffectPass {
public:
    ARTIFACT_CLASS();

    virtual int32_t GetOrderPreferenceIndex() const override { return -50; }

protected:
    virtual const char* GetShaderPath() const override { return "/Shaders/PostProcess/Tonemapping.glsl"; }
    virtual bool IsActive(const RenderContext& InContext) const override;
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) override;
};
