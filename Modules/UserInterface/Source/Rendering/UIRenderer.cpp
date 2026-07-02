#include "UIRenderer.h"
#include "UIDrawList.h"
#include "Assets/Font.h"

#include "Rendering/Surface.h"
#include "Rendering/Shader.h"
#include "Rendering/Sampler.h"
#include "Rendering/Texture.h"
#include "Rendering/Pipeline.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Buffer.h"
#include "Core/EngineConfig.h"
#include "Platform/FileIO.h"

#include <cstring>

// Maps world pixel-space (x in [0,W], y in [0,H], z = depth from tilt) to clip space with a
// perspective divide about the viewport centre. At z=0 it is the plain pixel->NDC mapping.
static Mat4 BuildUIProjection(float InWidth, float InHeight, float InPerspective) {
    Mat4 m(0.0f);
    m[0][0] = 2.0f / InWidth;         // clip.x = 2x/W - 1
    m[1][1] = 2.0f / InHeight;        // clip.y = 2y/H - 1
    m[3][0] = -1.0f;
    m[3][1] = -1.0f;
    m[2][3] = -1.0f / InPerspective;  // clip.w = 1 - z/P
    m[3][3] = 1.0f;
    return m;
}

void UIRenderer::CreateSharedResources() {
    m_SolidShader = Shader::Create(FileIO::ReadFileToString(EngineConfig::ContentDir() + "/Shaders/UISolid.glsl"));
    m_TextShader = Shader::Create(FileIO::ReadFileToString(EngineConfig::ContentDir() + "/Shaders/UIText.glsl"));

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

void UIRenderer::CreatePipelines(Surface* InTarget) {
    PipelineDesc solidDesc;
    solidDesc.Target = (Object*)InTarget;
    solidDesc.Shader = m_SolidShader;
    solidDesc.EnableBlending = true;
    solidDesc.EnableDepthTest = false;
    solidDesc.Buffers.Add(m_ProjectionBuffer);
    solidDesc.ImageBindings.Add({ 16, m_WhiteTexture->GetDefaultView(), m_Sampler });
    m_SolidPipeline = Pipeline::Create(solidDesc);

    m_TextPipelineReady = false;
    Font* font = UINode::GetDefaultFont();
    if (font && font->GetAtlasTexture()) {
        PipelineDesc textDesc;
        textDesc.Target = (Object*)InTarget;
        textDesc.Shader = m_TextShader;
        textDesc.EnableBlending = true;
        textDesc.EnableDepthTest = false;
        textDesc.Buffers.Add(m_ProjectionBuffer);
        textDesc.ImageBindings.Add({ 16, font->GetAtlasTexture()->GetDefaultView(), m_Sampler });
        m_TextPipeline = Pipeline::Create(textDesc);
        m_TextPipelineReady = true;
    }
}

void UIRenderer::Render(Surface* InTarget, UINode* InRoot, const Vec2& InViewportSize, const UIFrameContext& InContext) {
    if (!InTarget || !InRoot || InViewportSize.x <= 0.0f || InViewportSize.y <= 0.0f) {
        return;
    }
    if (!m_ResourcesReady) {
        CreateSharedResources();
    }

    const uint32_t width = (uint32_t)InViewportSize.x;
    const uint32_t height = (uint32_t)InViewportSize.y;

    // (Re)create pipelines on target/size change, or once the font atlas has finished loading.
    Font* font = UINode::GetDefaultFont();
    const bool needsFontPipeline = font && font->GetAtlasTexture() && !m_TextPipelineReady;
    if (!m_SolidPipeline || m_CachedTarget != InTarget || m_CachedWidth != width || m_CachedHeight != height || needsFontPipeline) {
        CreatePipelines(InTarget);
        m_CachedTarget = InTarget;
        m_CachedWidth = width;
        m_CachedHeight = height;
    }

    // Upload the perspective projection for this frame; keep hit-testing in sync with it.
    UINode::SetViewport(InViewportSize.x, InViewportSize.y);
    const Mat4 projection = BuildUIProjection(InViewportSize.x, InViewportSize.y, UINode::GetPerspective());
    void* mapped = m_ProjectionBuffer->MapData(sizeof(Mat4), 0);
    memcpy(mapped, &projection, sizeof(Mat4));
    m_ProjectionBuffer->UnmapData();

    InRoot->BindTree();
    InRoot->Layout(UIRectF(Vec2(0.0f), InViewportSize));
    InRoot->UpdateTree(InContext);

    UIDrawList drawList;
    InRoot->PaintTree(drawList);

    if (drawList.IsEmpty()) {
        return;
    }

    // One shared buffer; draw each batch in paint (tree) order so later nodes layer in front.
    m_VertexBuffer = VertexBuffer::Create(drawList.GetVertices(), drawList.GetIndices());
    for (const UIDrawList::Batch& batch : drawList.GetBatches()) {
        if (batch.Kind == UIDrawList::BatchKind::Text) {
            if (!m_TextPipelineReady) {
                continue;
            }
            m_TextPipeline->Bind();
        } else {
            m_SolidPipeline->Bind();
        }
        m_VertexBuffer->Draw(batch.IndexCount, batch.FirstIndex);
    }
}
