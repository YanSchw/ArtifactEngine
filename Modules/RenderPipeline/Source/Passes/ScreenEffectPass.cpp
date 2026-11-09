#include "ScreenEffectPass.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"

void ScreenEffectPass::Initialize() {
    m_Shader = ShaderLibrary::CreateShader(GetShaderPath());
    m_LinearSampler = Sampler::Create({ FilterMode::Linear, FilterMode::Linear,
                                        AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
}

void ScreenEffectPass::DrawEffect(ImageView& InSource, FrameBuffer& InTarget) {
    PipelineDesc desc;
    desc.Target = &InTarget;
    desc.Shader = m_Shader;
    desc.Buffers.Add(m_Uniforms);
    desc.ImageBindings.Add({ 16, &InSource, m_LinearSampler });
    m_Quad.Draw(desc);
}
