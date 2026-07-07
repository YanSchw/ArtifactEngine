#include "UIRenderer.h"
#include "UIDrawList.h"
#include "Assets/Font.h"
#include "GameFramework/UICanvas.h"

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

void UIRenderer::CreateSharedResources() {
    m_SolidShader = Shader::Create(FileIO::ReadFileToString(EngineConfig::GetContentDir("UserInterface") + "/Shaders/UISolid.glsl"));
    m_TextShader = Shader::Create(FileIO::ReadFileToString(EngineConfig::GetContentDir("UserInterface") + "/Shaders/UIText.glsl"));

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

void UIRenderer::Render(Surface* InTarget, UICanvas* InCanvas, const Vec2& InViewportSize, const UIFrameContext& InContext) {
    if (!InTarget || !InCanvas || InViewportSize.x <= 0.0f || InViewportSize.y <= 0.0f) {
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

    // The canvas runs the frame (bind/layout/input/paint) and hands back the projection to draw with.
    UIDrawList drawList;
    const Mat4 projection = InCanvas->RunFrame(InViewportSize, InContext, drawList);
    void* mapped = m_ProjectionBuffer->MapData(sizeof(Mat4), 0);
    memcpy(mapped, &projection, sizeof(Mat4));
    m_ProjectionBuffer->UnmapData();

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
