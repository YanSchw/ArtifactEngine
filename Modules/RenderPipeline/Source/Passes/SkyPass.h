#pragma once
#include "Rendering/RenderPass.h"
#include "Rendering/FullscreenQuad.h"
#include "SkyPass.gen.h"

class Sampler;
class Shader;
class UniformBuffer;

class SkyPass : public RenderPass {
public:
    ARTIFACT_CLASS();

    static constexpr uint32_t ModeCount = 2;

    virtual RenderStageName GetStageName() const override { return RenderStageName::Forward; }
    virtual int32_t GetOrderPreferenceIndex() const override { return -1000; }

protected:
    virtual void Initialize() override;
    virtual void Execute(const RenderContext& InContext) override;

private:
    SharedObjectPtr<Shader> m_SkyShaders[ModeCount];
    SharedObjectPtr<Sampler> m_PanoramaSampler;
    SharedObjectPtr<UniformBuffer> m_Uniforms;
    FullscreenQuad m_SkyQuads[ModeCount];
};
