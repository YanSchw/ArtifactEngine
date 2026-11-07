#pragma once
#include "Rendering/RenderPass.h"
#include "Rendering/FullscreenQuad.h"
#include "DeferredLightPass.gen.h"

class FrameBuffer;
class Sampler;
class Shader;

/** Lights the DeferredGeometry stage's GBuffer with one fullscreen draw, added onto the stage's target. */
class DeferredLightPass : public RenderPass {
public:
    ARTIFACT_CLASS();

    /** The bindings /Shaders/Deferred/Common.glsl reads, drawing into InTarget. */
    static PipelineDesc DescribeGBufferRead(FrameBuffer& InTarget, FrameBuffer& InGBuffer, Shader& InShader,
                                            Sampler& InSampler, const RenderContext& InContext);

protected:
    virtual const char* GetShaderKey() const = 0;
    virtual void Initialize() override;
    virtual void Execute(const RenderContext& InContext) override;

private:
    SharedObjectPtr<Shader> m_Shader;
    SharedObjectPtr<Sampler> m_Sampler;
    FullscreenQuad m_Quad;
};

class DeferredDirectionalLightPass : public DeferredLightPass {
public:
    ARTIFACT_CLASS();

protected:
    virtual const char* GetShaderKey() const override { return "/Shaders/Deferred/DirectionalLight.glsl"; }
};

class DeferredPointLightPass : public DeferredLightPass {
public:
    ARTIFACT_CLASS();

protected:
    virtual const char* GetShaderKey() const override { return "/Shaders/Deferred/PointLights.glsl"; }
    virtual void Execute(const RenderContext& InContext) override;
};
