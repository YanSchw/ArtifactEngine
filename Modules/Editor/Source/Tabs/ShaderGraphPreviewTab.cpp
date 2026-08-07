#include "ShaderGraphPreviewTab.h"

#include "Assets/AssetManager.h"
#include "Assets/Mesh.h"
#include "Assets/Texture2D.h"
#include "Assets/ShaderGraph.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Image.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderTargetTexture.h"
#include "Rendering/Sampler.h"
#include "Rendering/Texture.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderData.h"
#include "Rendering/ShaderTemplate.h"
#include "Rendering/VertexBuffer.h"
#include "GameFramework/UIImage.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"

static const UUID s_PreviewMesh = UUID::FromString("c6308770-3a5b-4b2b-9cec-14ba803ff817");
static const Vec4 s_ClearColor = HexColor(0x141417);

struct PreviewSceneData {
    Mat4 ViewProjection = Mat4(1.0f);
    float Time = 0.0f;
    float Padding[3] = { 0.0f, 0.0f, 0.0f };
};

struct PreviewPushData {
    Mat4 WorldTransform = Mat4(1.0f);
    uint32_t NodeId = 0;
    uint32_t Padding[3] = { 0, 0, 0 };
};

ShaderGraphPreviewTab::ShaderGraphPreviewTab() {
    m_Texture = Object::Create<RenderTargetTexture>();

    m_Image = Add<UIImage>();
    m_Image->Fill();
    m_Image->Image = m_Texture;

    SamplerDesc samplerDesc;
    samplerDesc.MinFilter = FilterMode::Linear;
    samplerDesc.MagFilter = FilterMode::Linear;
    m_Sampler = Sampler::Create(samplerDesc);

    m_SceneBuffer = UniformBuffer::Create(0, sizeof(PreviewSceneData));
}

VectorImage* ShaderGraphPreviewTab::GetTabIcon() const {
    return EditorIcons::Viewport();
}

void ShaderGraphPreviewTab::SetShaderGraph(ShaderGraph* InShaderGraph) {
    m_ShaderGraph = InShaderGraph;
    InvalidatePipeline();
}

void ShaderGraphPreviewTab::InvalidatePipeline() {
    m_PipelineDirty = true;
}

void ShaderGraphPreviewTab::EnsureTarget(uint32_t InWidth, uint32_t InHeight) {
    if (m_Target && m_Width == InWidth && m_Height == InHeight) {
        return;
    }
    if (m_Target) {
        RenderingAPI::GetInstance()->WaitIdle();
    }

    m_Width = InWidth;
    m_Height = InHeight;

    auto makeColor = [&](ImageFormat InFormat) {
        ImageDesc imageDesc;
        imageDesc.Width = InWidth;
        imageDesc.Height = InHeight;
        imageDesc.Format = InFormat;
        imageDesc.Usage = ImageUsage::ColorAttachment | ImageUsage::Sampled;

        ImageViewDesc viewDesc;
        viewDesc.ImagePtr = Image::Create(imageDesc);
        viewDesc.Format = InFormat;
        return ImageView::Create(viewDesc);
    };

    ImageDesc depthDesc;
    depthDesc.Width = InWidth;
    depthDesc.Height = InHeight;
    depthDesc.Format = ImageFormat::Depth32F;
    depthDesc.Usage = ImageUsage::DepthStencil;

    ImageViewDesc depthViewDesc;
    depthViewDesc.ImagePtr = Image::Create(depthDesc);
    depthViewDesc.Format = ImageFormat::Depth32F;
    depthViewDesc.Aspect = ImageAspect::Depth;

    FrameBufferDesc targetDesc;
    targetDesc.Width = InWidth;
    targetDesc.Height = InHeight;
    targetDesc.Samples = SampleCount::X4;
    targetDesc.ColorAttachments.Add(makeColor(ImageFormat::RGBA8));
    targetDesc.ColorAttachments.Add(makeColor(ImageFormat::RGBA8));
    targetDesc.DepthAttachment = ImageView::Create(depthViewDesc);
    targetDesc.ClearColor = s_ClearColor;
    targetDesc.ClearColors.Add(s_ClearColor);
    targetDesc.ClearColors.Add(Vec4(0.0f));

    m_Target = FrameBuffer::Create(targetDesc);
    m_Texture->SetView(m_Target->GetDesc().ColorAttachments[0]);
    m_PipelineDirty = true;
}

void ShaderGraphPreviewTab::EnsurePipeline() {
    ShaderGraph* graph = m_ShaderGraph.Get();
    if (!m_PipelineDirty || !m_Target || !graph) {
        return;
    }
    Shader* shader = graph->GetShader();
    if (!shader) {
        m_Pipeline = nullptr;
        return;
    }

    // A pipeline missing a binding the shader declares is a descriptor-set mismatch, so stay dirty
    // and retry until every texture has streamed in.
    PipelineDesc desc;
    for (const ShaderGraphTextureBinding& binding : graph->GetTextureBindings()) {
        AssetManager::Get().LoadAsset(binding.Texture);
        Texture* texture = binding.Texture->GetTexture();
        if (!texture) {
            return;
        }
        desc.ImageBindings.Add({ binding.Binding, texture->GetDefaultView(), m_Sampler });
    }

    m_PipelineDirty = false;

    if (m_Pipeline) {
        RenderingAPI::GetInstance()->WaitIdle();
    }

    desc.Target = m_Target;
    desc.Shader = shader;
    desc.Buffers.Add(m_SceneBuffer);
    if (UniformBuffer* properties = graph->GetPropertyBuffer()) {
        desc.Buffers.Add(properties);
    }
    m_Pipeline = Pipeline::Create(desc);
}

void ShaderGraphPreviewTab::UpdateSceneBuffer(float InDeltaTime) {
    m_Time += InDeltaTime;

    const float aspect = m_Height > 0 ? (float)m_Width / (float)m_Height : 1.0f;
    Mat4 projection = glm::perspectiveLH(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    projection[1][1] *= -1.0f;
    const Mat4 view = glm::lookAtLH(Vec3(0.0f, 1.6f, -3.2f), Vec3(0.0f), VecUtils::Up);

    PreviewSceneData data;
    data.ViewProjection = projection * view;
    data.Time = m_Time;

    void* mapped = m_SceneBuffer->MapData(sizeof(data), 0);
    memcpy(mapped, &data, sizeof(data));
    m_SceneBuffer->UnmapData();
}

void ShaderGraphPreviewTab::OnUIUpdate(const UIFrameContext& InContext) {
    MinorTab::OnUIUpdate(InContext);

    const UIRectF rect = GetGeometry();
    const uint32_t width = (uint32_t)glm::max(rect.Size.x, 1.0f);
    const uint32_t height = (uint32_t)glm::max(rect.Size.y, 1.0f);
    if (width <= 1 || height <= 1) {
        return;
    }

    if (!m_Mesh) {
        if (Mesh* mesh = AssetManager::Get().GetAsset<Mesh>(s_PreviewMesh)) {
            AssetManager::Get().LoadAsset(mesh);
            m_Mesh = mesh->GetVertexBuffer();
        }
    }

    EnsureTarget(width, height);
    EnsurePipeline();
    if (!m_Pipeline || !m_Mesh) {
        return;
    }

    UpdateSceneBuffer((float)InContext.DeltaTime);

    PreviewPushData push;
    push.WorldTransform = glm::rotate(Mat4(1.0f), m_Time * 0.6f, VecUtils::Up);

    if (!m_ShaderData) {
        m_ShaderData = new ShaderData();
    }
    m_ShaderData->Set(push);

    m_Pipeline->Bind();
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::SetShaderData,
                                                       CmdSetShaderData{ m_ShaderData.Get() });
    m_Mesh->Draw();

    m_Texture->SetView(m_Target->GetDesc().ColorAttachments[0]);
}
