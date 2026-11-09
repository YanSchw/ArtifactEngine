#include "MaterialPipelineCache.h"
#include "Assets/Material.h"
#include "Assets/Texture2D.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderPass.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderTemplate.h"
#include "Rendering/Texture.h"

MaterialPipelineCache::MaterialPipelineCache(const Array<ShaderDataType>& InVertexLayout) : m_VertexLayout(InVertexLayout) {
    SamplerDesc samplerDesc;
    samplerDesc.MagFilter = FilterMode::Linear;
    samplerDesc.MinFilter = FilterMode::Linear;
    m_Sampler = Sampler::Create(samplerDesc);
}

MaterialPipelineCache::~MaterialPipelineCache() = default;

Pipeline* MaterialPipelineCache::Resolve(Material& InMaterial, FrameBuffer& InTarget, const RenderContext& InContext) {
    Shader* shader = InMaterial.GetShader();
    Array<void*> resources = { &InTarget, shader, InMaterial.GetPropertyBuffer(), InContext.SceneUniforms, InContext.ShadowMap };
    Array<SampledImageBinding> imageBindings;
    imageBindings.Add({ ShaderTemplate::ShadowMapBinding, InContext.ShadowMap, InContext.ShadowSampler });
    for (const MaterialTextureBinding& binding : InMaterial.GetTextureBindings()) {
        Texture* texture = binding.Texture ? binding.Texture->GetTexture() : nullptr;
        if (!texture) {
            return nullptr;
        }
        resources.Add(texture);
        imageBindings.Add({ binding.Binding, texture->GetDefaultView(), m_Sampler });
    }

    Entry& cached = m_Entries[InMaterial.GetId()];
    if (cached.Instance && cached.Resources == resources) {
        return cached.Instance.Get();
    }

    PipelineDesc desc;
    desc.Target = &InTarget;
    desc.Shader = shader;
    desc.VertexLayout = m_VertexLayout;
    desc.Buffers.Add(InContext.SceneUniforms);
    if (UniformBuffer* properties = InMaterial.GetPropertyBuffer()) {
        desc.Buffers.Add(properties);
    }
    desc.ImageBindings = imageBindings;

    cached.Instance = Pipeline::Create(desc);
    cached.Resources = resources;
    return cached.Instance.Get();
}
