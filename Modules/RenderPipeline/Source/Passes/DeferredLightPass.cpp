#include "DeferredLightPass.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/GBuffer.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/ShaderTemplate.h"
#include "GameFramework/World.h"
#include "GameFramework/PointLightNode.h"
#include "GameFramework/AmbientLightNode.h"

template<typename T>
static bool HasEnabledNode(World* InWorld) {
    if (!InWorld) {
        return false;
    }
    for (Node* node : InWorld->GetAllNodes()) {
        T* typed = node->As<T>();
        if (typed && typed->IsEnabled()) {
            return true;
        }
    }
    return false;
}

PipelineDesc DeferredLightPass::DescribeGBufferRead(FrameBuffer& InTarget, FrameBuffer& InGBuffer, Shader& InShader,
                                                    Sampler& InSampler, const RenderContext& InContext) {
    PipelineDesc desc;
    desc.Target = &InTarget;
    desc.Shader = &InShader;
    desc.Buffers.Add(InContext.SceneUniforms);
    desc.ImageBindings.Add({ ShaderTemplate::ShadowMapBinding, InContext.ShadowMap, InContext.ShadowSampler });

    const FrameBufferDesc& gbuffer = InGBuffer.GetDesc();
    for (int32_t attachment = 0; attachment < GBufferLayout::AttachmentCount; attachment++) {
        desc.ImageBindings.Add({ GBufferLayout::FirstBinding + attachment, gbuffer.ColorAttachments[attachment], &InSampler });
    }
    desc.ImageBindings.Add({ GBufferLayout::DepthBinding, gbuffer.DepthAttachment, &InSampler });
    return desc;
}

void DeferredLightPass::Initialize() {
    m_Shader = ShaderLibrary::CreateShader(GetShaderKey());
    m_Sampler = Sampler::Create({ FilterMode::Nearest, FilterMode::Nearest,
                                  AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
}

void DeferredLightPass::Execute(const RenderContext& InContext) {
    RenderStage* geometry = GetStage()->GetPipeline()->FindStage(RenderStageName::DeferredGeometry);
    FrameBuffer* gbuffer = geometry ? geometry->GetTarget() : nullptr;
    if (!gbuffer || !m_Shader) {
        return;
    }
    m_Quad.Draw(DescribeGBufferRead(*GetStage()->GetTarget(), *gbuffer, *m_Shader, *m_Sampler, InContext));
}

void DeferredAmbientLightPass::Execute(const RenderContext& InContext) {
    if (HasEnabledNode<AmbientLightNode>(InContext.World)) {
        DeferredLightPass::Execute(InContext);
    }
}

void DeferredPointLightPass::Execute(const RenderContext& InContext) {
    if (HasEnabledNode<PointLightNode>(InContext.World)) {
        DeferredLightPass::Execute(InContext);
    }
}
