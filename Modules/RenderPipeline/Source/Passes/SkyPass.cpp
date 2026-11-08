#include "SkyPass.h"
#include "Assets/Texture2D.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/RenderStage.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/Texture.h"
#include "GameFramework/SkyboxNode.h"

#include <cstring>

static constexpr uint32_t s_PanoramaBinding = 16;

static const char* s_SkyShaderKeys[SkyPass::ModeCount] = {
    "/Shaders/Sky/Gradient.glsl",
    "/Shaders/Sky/Hdri.glsl"
};

struct SkyUniforms {
    Vec4 GradientTop = Vec4(0.0f);
    Vec4 GradientHorizon = Vec4(0.0f);
    Vec4 GradientBottom = Vec4(0.0f);
    Vec4 Params = Vec4(0.0f);
};

void SkyPass::Initialize() {
    for (uint32_t mode = 0; mode < ModeCount; mode++) {
        m_SkyShaders[mode] = ShaderLibrary::CreateShader(s_SkyShaderKeys[mode]);
    }
    m_PanoramaSampler = Sampler::Create({ FilterMode::Linear, FilterMode::Linear,
                                          AddressMode::Repeat, AddressMode::Clamp, AddressMode::Clamp });
    m_Uniforms = UniformBuffer::Create(1, sizeof(SkyUniforms));
}

void SkyPass::Execute(const RenderContext& InContext) {
    SkyboxNode* sky = SkyboxNode::FindActive(InContext.World);
    if (!sky || !InContext.SceneUniforms) {
        return;
    }
    const uint32_t mode = (uint32_t)sky->GetMode();
    if (mode >= ModeCount || !m_SkyShaders[mode]) {
        return;
    }

    PipelineDesc desc;
    desc.Target = GetStage()->GetTarget();
    desc.Shader = m_SkyShaders[mode];
    desc.Buffers.Add(InContext.SceneUniforms);
    desc.Buffers.Add(m_Uniforms);
    if (sky->GetMode() == SkyboxMode::Hdri) {
        Texture2D* panorama = sky->GetHdri().Texture.Get();
        Texture* texture = panorama ? panorama->GetTexture() : nullptr;
        if (!texture) {
            return;
        }
        desc.ImageBindings.Add({ s_PanoramaBinding, texture->GetDefaultView(), m_PanoramaSampler });
    }

    const SkyGradientParams& gradient = sky->GetGradient();
    SkyUniforms uniforms;
    uniforms.GradientTop = gradient.Top;
    uniforms.GradientHorizon = gradient.Horizon;
    uniforms.GradientBottom = gradient.Bottom;
    uniforms.Params = Vec4(glm::max(sky->GetIntensity(), 0.0f), glm::radians(sky->GetHdri().Rotation), 0.0f, 0.0f);
    void* mapped = m_Uniforms->MapData(sizeof(uniforms), 0);
    memcpy(mapped, &uniforms, sizeof(uniforms));
    m_Uniforms->UnmapData();

    m_SkyQuads[mode].Draw(desc);
}
