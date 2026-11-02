#include "UIRenderer.h"
#include "UIDrawList.h"
#include "Assets/Font.h"
#include "GameFramework/UICanvas.h"

#include "Rendering/Surface.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/Sampler.h"
#include "Rendering/Texture.h"
#include "Rendering/Pipeline.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Buffer.h"

#include <cstring>

void UIRenderer::CreateSharedResources() {
    m_SolidShader = ShaderLibrary::CreateShader("/Shaders/UISolid.glsl");
    m_TextShader = ShaderLibrary::CreateShader("/Shaders/UIText.glsl");
    m_ImageShader = ShaderLibrary::CreateShader("/Shaders/UIImage.glsl");

    SamplerDesc samplerDesc;
    samplerDesc.MinFilter = FilterMode::Linear;
    samplerDesc.MagFilter = FilterMode::Linear;
    samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = AddressMode::Clamp;
    m_Sampler = Sampler::Create(samplerDesc);

    byte whitePixel[4] = { 255, 255, 255, 255 };
    TextureDesc whiteDesc;
    whiteDesc.Width = 1;
    whiteDesc.Height = 1;
    whiteDesc.GenerateMips = false;
    m_WhiteTexture = Texture::Create(whitePixel, 1, 1, 4, whiteDesc);

    m_ProjectionBuffer = UniformBuffer::Create(0, sizeof(Mat4));

    m_ResourcesReady = true;
}

void UIRenderer::CreatePipelines(Object* InTarget) {
    // Text and image pipelines are built lazily per texture against the current target.
    m_TextPipelines = PipelineCache();
    m_ImagePipelines = PipelineCache();

    PipelineDesc solidDesc;
    solidDesc.Target = InTarget;
    solidDesc.Shader = m_SolidShader;
    solidDesc.VertexLayout = UIVertex::GetLayout();
    solidDesc.Buffers.Add(m_ProjectionBuffer);
    solidDesc.ImageBindings.Add({ 16, m_WhiteTexture->GetDefaultView(), m_Sampler });
    m_SolidPipeline = Pipeline::Create(solidDesc);
}

void UIRenderer::Render(Object* InTarget, UICanvas* InCanvas, const Vec2& InViewportSize, const UIFrameContext& InContext) {
    if (!InTarget || !InCanvas || InViewportSize.x <= 0.0f || InViewportSize.y <= 0.0f) {
        return;
    }
    UIDrawList drawList;
    const Mat4 projection = InCanvas->RunFrame(InViewportSize, InContext, drawList);
    Submit(InTarget, InViewportSize, drawList, projection);
}

void UIRenderer::Submit(Object* InTarget, const Vec2& InViewportSize, const UIDrawList& InDrawList, const Mat4& InProjection) {
    if (!InTarget || InViewportSize.x <= 0.0f || InViewportSize.y <= 0.0f) {
        return;
    }
    if (!m_ResourcesReady) {
        CreateSharedResources();
    }

    const uint32_t width = (uint32_t)InViewportSize.x;
    const uint32_t height = (uint32_t)InViewportSize.y;

    // (Re)create pipelines on target/size change.
    if (!m_SolidPipeline || m_CachedTarget != InTarget || m_CachedWidth != width || m_CachedHeight != height) {
        CreatePipelines(InTarget);
        m_CachedTarget = InTarget;
        m_CachedWidth = width;
        m_CachedHeight = height;
    }

    void* mapped = m_ProjectionBuffer->MapData(sizeof(Mat4), 0);
    memcpy(mapped, &InProjection, sizeof(Mat4));
    m_ProjectionBuffer->UnmapData();

    m_SolidPipeline->Bind();
    if (InDrawList.IsEmpty()) {
        return;
    }

    if (!m_DynamicVertexBuffer) {
        m_DynamicVertexBuffer = VertexBuffer::CreateDynamic();
    }
    m_DynamicVertexBuffer->Update(&InDrawList.GetVertices()[0], (uint32_t)(InDrawList.GetVertices().Size() * sizeof(UIVertex)), InDrawList.GetIndices());

    // Draw each batch in paint (tree) order so later nodes layer in front.
    for (const UIDrawList::Batch& batch : InDrawList.GetBatches()) {
        if (batch.Kind == UIDrawList::BatchKind::Text) {
            Texture* atlas = batch.Tex;
            if (!atlas) {
                Font* defaultFont = UINode::GetDefaultFont();
                atlas = defaultFont ? defaultFont->GetAtlasTexture() : nullptr;
            }
            Pipeline* pipeline = GetTexturedPipeline(m_TextPipelines, m_TextShader, atlas);
            if (!pipeline) {
                continue;
            }
            pipeline->Bind();
        } else if (batch.Kind == UIDrawList::BatchKind::Image) {
            Pipeline* pipeline = GetTexturedPipeline(m_ImagePipelines, m_ImageShader, batch.Tex);
            if (!pipeline) {
                continue;
            }
            pipeline->Bind();
        } else {
            m_SolidPipeline->Bind();
        }
        m_DynamicVertexBuffer->Draw(batch.IndexCount, batch.FirstIndex);
    }
}

Pipeline* UIRenderer::GetTexturedPipeline(PipelineCache& InCache, const SharedObjectPtr<Shader>& InShader, Texture* InTexture) {
    if (!InTexture || !m_CachedTarget) {
        return nullptr;
    }
    SharedObjectPtr<ImageView> view = InTexture->GetDefaultView();
    if (!view.Get()) {
        return nullptr;
    }
    if (InCache.ContainsKey(InTexture)) {
        Pipeline* cached = InCache[InTexture].Get();
        if (std::get<1>(cached->GetDesc().ImageBindings[0]).Get() == view.Get()) {
            return cached;
        }
    }
    PipelineDesc desc;
    desc.Target = (Object*)m_CachedTarget;
    desc.Shader = InShader;
    desc.VertexLayout = UIVertex::GetLayout();
    desc.Buffers.Add(m_ProjectionBuffer);
    desc.ImageBindings.Add({ 16, view, m_Sampler });
    SharedObjectPtr<Pipeline> pipeline = Pipeline::Create(desc);
    InCache[InTexture] = pipeline;
    return pipeline.Get();
}
