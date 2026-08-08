#include "AssetPreviewTab.h"

#include "Assets/AssetManager.h"
#include "Assets/Material.h"
#include "Assets/Mesh.h"
#include "Assets/Texture2D.h"
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
#include "Rendering/SceneUniforms.h"
#include "Rendering/ShaderTemplate.h"
#include "Rendering/VertexBuffer.h"
#include "GameFramework/UIImage.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"

static const UUID s_DefaultMesh = UUID::FromString("c6308770-3a5b-4b2b-9cec-14ba803ff817");
static const UUID s_DefaultShaderGraph = UUID::FromString("d351ca39-9ab6-43cf-9921-4965ec126be8");
static const Vec4 s_ClearColor = HexColor(0x141417);

static const Vec3 s_KeyLightDirection = glm::normalize(Vec3(0.45f, -0.8f, 0.4f));

struct PreviewPushData {
    Mat4 WorldTransform = Mat4(1.0f);
    uint32_t NodeId = 0;
    uint32_t Padding[3] = { 0, 0, 0 };
};

AssetPreviewTab::AssetPreviewTab() {
    m_Texture = Object::Create<RenderTargetTexture>();

    m_Image = Add<UIImage>();
    m_Image->Fill();
    m_Image->Image = m_Texture;

    SamplerDesc samplerDesc;
    samplerDesc.MinFilter = FilterMode::Linear;
    samplerDesc.MagFilter = FilterMode::Linear;
    m_Sampler = Sampler::Create(samplerDesc);

    m_SceneBuffer = UniformBuffer::Create(0, sizeof(SceneUniformData));
    m_ShadowMap.Create();
}

VectorImage* AssetPreviewTab::GetTabIcon() const {
    return EditorIcons::Viewport();
}

void AssetPreviewTab::SetMaterial(Material* InMaterial) {
    m_Material = InMaterial;
    AssetManager::Get().LoadAsset(InMaterial);
    InvalidatePipeline();
}

void AssetPreviewTab::SetMesh(Mesh* InMesh) {
    m_Mesh = InMesh;
    AssetManager::Get().LoadAsset(InMesh);
    InvalidatePipeline();
}

void AssetPreviewTab::InvalidatePipeline() {
    m_PipelineDirty = true;
}

Material* AssetPreviewTab::ResolveMaterial() const {
    if (Material* material = m_Material.Get()) {
        return material;
    }
    Mesh* mesh = ResolveMesh();
    Material* material = mesh ? mesh->GetMaterial() : nullptr;
    return material ? material : AssetManager::Get().GetAsset<Material>(s_DefaultShaderGraph);
}

Mesh* AssetPreviewTab::ResolveMesh() const {
    Mesh* mesh = m_Mesh.Get();
    return mesh ? mesh : AssetManager::Get().GetAsset<Mesh>(s_DefaultMesh);
}

void AssetPreviewTab::EnsureTarget(uint32_t InWidth, uint32_t InHeight) {
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

void AssetPreviewTab::EnsurePipeline() {
    Material* material = ResolveMaterial();
    Shader* shader = material ? material->GetShader() : nullptr;
    if (!m_Target || !shader) {
        m_Pipeline = nullptr;
        return;
    }

    // A pipeline missing a binding the shader declares is a descriptor-set mismatch, so keep the
    // previous one until every texture has streamed in.
    Array<void*> resources = { shader, material->GetPropertyBuffer() };
    PipelineDesc desc;
    desc.ImageBindings.Add({ ShaderTemplate::ShadowMapBinding, m_ShadowMap.GetView(), m_ShadowMap.GetSampler() });
    for (const MaterialTextureBinding& binding : material->GetTextureBindings()) {
        AssetManager::Get().LoadAsset(binding.Texture);
        Texture* texture = binding.Texture ? binding.Texture->GetTexture() : nullptr;
        if (!texture) {
            return;
        }
        resources.Add(texture);
        desc.ImageBindings.Add({ binding.Binding, texture->GetDefaultView(), m_Sampler });
    }

    if (!m_PipelineDirty && m_Pipeline && m_PipelineResources == resources) {
        return;
    }

    if (m_Pipeline) {
        RenderingAPI::GetInstance()->WaitIdle();
    }

    desc.Target = m_Target;
    desc.Shader = shader;
    desc.Buffers.Add(m_SceneBuffer);
    if (UniformBuffer* properties = material->GetPropertyBuffer()) {
        desc.Buffers.Add(properties);
    }

    m_Pipeline = Pipeline::Create(desc);
    m_PipelineResources = resources;
    m_PipelineDirty = false;
}

void AssetPreviewTab::UpdateSceneBuffer(float InDeltaTime, const Mesh& InMesh) {
    m_Time += InDeltaTime;

    const float radius = InMesh.GetBoundsRadius();
    const float aspect = m_Height > 0 ? (float)m_Width / (float)m_Height : 1.0f;
    Mat4 projection = glm::perspectiveLH(glm::radians(45.0f), aspect, radius * 0.01f, radius * 50.0f);
    projection[1][1] *= -1.0f;
    const Mat4 view = glm::lookAtLH(Vec3(0.0f, radius * 0.9f, -radius * 3.2f), Vec3(0.0f), VecUtils::Up);

    SceneUniformData data;
    data.ViewProjection = projection * view;
    data.Time = m_Time;
    data.SunDirection = Vec4(s_KeyLightDirection, 0.0f);
    data.SunColor = Vec4(1.15f, 1.13f, 1.06f, 0.0f);
    data.AmbientColor = Vec4(0.16f, 0.19f, 0.23f, 0.0f);

    void* mapped = m_SceneBuffer->MapData(sizeof(data), 0);
    memcpy(mapped, &data, sizeof(data));
    m_SceneBuffer->UnmapData();
}

void AssetPreviewTab::OnUIUpdate(const UIFrameContext& InContext) {
    MinorTab::OnUIUpdate(InContext);

    const UIRectF rect = GetGeometry();
    const uint32_t width = (uint32_t)glm::max(rect.Size.x, 1.0f);
    const uint32_t height = (uint32_t)glm::max(rect.Size.y, 1.0f);
    if (width <= 1 || height <= 1) {
        return;
    }

    Mesh* mesh = ResolveMesh();
    if (mesh) {
        AssetManager::Get().LoadAsset(mesh);
    }
    VertexBuffer* vertexBuffer = mesh ? mesh->GetVertexBuffer() : nullptr;

    EnsureTarget(width, height);
    EnsurePipeline();
    if (!m_Pipeline || !vertexBuffer) {
        return;
    }

    m_ShadowMap.Clear();

    UpdateSceneBuffer((float)InContext.DeltaTime, *mesh);

    PreviewPushData push;
    push.WorldTransform = glm::rotate(Mat4(1.0f), m_Time * 0.6f, VecUtils::Up)
                        * glm::translate(Mat4(1.0f), -mesh->GetBoundsCenter());

    if (!m_ShaderData) {
        m_ShaderData = new ShaderData();
    }
    m_ShaderData->Set(push);

    m_Pipeline->Bind();
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::SetShaderData,
                                                       CmdSetShaderData{ m_ShaderData.Get() });
    vertexBuffer->Draw();

    m_Texture->SetView(m_Target->GetDesc().ColorAttachments[0]);
}
