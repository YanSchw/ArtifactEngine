#include "CoreMinimal.h"
#include "Platform/Platform.h"

#include "ArtifactRenderPipeline.h"
#include "Assets/AssetManager.h"
#include "Assets/Material.h"
#include "Assets/Texture2D.h"
#include "Assets/Mesh.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/Sampler.h"
#include "Rendering/Buffer.h"
#include "Rendering/Pipeline.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Image.h"
#include "Rendering/ShaderData.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/StaticMeshNode.h"

/** Mirrors the SceneBlock every shader graph template declares. */
struct SceneUniformData {
    Mat4 ViewProjection = Mat4(1.0f);
    float Time = 0.0f;
    float Padding[3] = { 0.0f, 0.0f, 0.0f };
};

void ArtifactRenderPipeline::UpdateUniformData(double InDeltaTime, const RenderParams& InParams) {
    m_Time += (float)InDeltaTime;

    CameraNode* camera = InParams.CameraOverride;
    if (!camera && InParams.m_World) {
        camera = InParams.m_World->GetMainCamera();
    }

    SceneUniformData data;
    data.Time = m_Time;
    if (camera) {
        // A zero height (minimized window) would make this NaN
        camera->SetAspectRatio(InParams.Width / (float) glm::max(InParams.Height, 1u));
        data.ViewProjection = camera->GetViewProjectionMatrix();
    }

    void* mapped = m_UniformBuffer->MapData(sizeof(data), 0);
    memcpy(mapped, &data, sizeof(data));
    m_UniformBuffer->UnmapData();
}

Pipeline* ArtifactRenderPipeline::ResolvePipeline(Material* InMaterial) {
    if (!InMaterial) {
        return nullptr;
    }

    AssetManager::Get().LoadAsset(InMaterial);
    Shader* shader = InMaterial->GetShader();
    if (!shader) {
        return nullptr;
    }

    // A pipeline missing a binding its shader declares is a descriptor-set mismatch, so the mesh
    // stays unrendered until every texture has streamed in.
    Array<void*> resources = { shader, InMaterial->GetPropertyBuffer() };
    Array<std::tuple<uint32_t, SharedObjectPtr<ImageView>, SharedObjectPtr<Sampler>>> imageBindings;
    for (const MaterialTextureBinding& binding : InMaterial->GetTextureBindings()) {
        AssetManager::Get().LoadAsset(binding.Texture);
        Texture* texture = binding.Texture ? binding.Texture->GetTexture() : nullptr;
        if (!texture) {
            return nullptr;
        }
        resources.Add(texture);
        imageBindings.Add({ binding.Binding, texture->GetDefaultView(), m_Sampler });
    }

    MaterialPipeline& cached = m_MaterialPipelines[InMaterial->GetId()];
    if (cached.Instance.Get() && cached.Resources == resources) {
        return cached.Instance.Get();
    }

    PipelineDesc desc;
    desc.Target = m_FrameBuffer;
    desc.Shader = shader;
    desc.Buffers.Add(m_UniformBuffer);
    if (UniformBuffer* properties = InMaterial->GetPropertyBuffer()) {
        desc.Buffers.Add(properties);
    }
    desc.ImageBindings = imageBindings;

    cached.Instance = Pipeline::Create(desc);
    cached.Resources = resources;
    return cached.Instance.Get();
}

void ArtifactRenderPipeline::Invalidate(uint32_t InWidth, uint32_t InHeight) {
    if (m_FrameBuffer.Get()) {
        RenderingAPI::GetInstance()->WaitIdle();
    }

    m_Width = InWidth;
    m_Height = InHeight;

    ImageDesc imageDesc;
    imageDesc.Width = InWidth;
    imageDesc.Height = InHeight;
    imageDesc.Format = ImageFormat::RGBA8;
    imageDesc.Usage = ImageUsage::ColorAttachment | ImageUsage::Sampled;
    auto image = Image::Create(imageDesc);
    ImageViewDesc imageViewDesc;
    imageViewDesc.ImagePtr = image;
    imageViewDesc.Format = ImageFormat::RGBA8;
    auto imageView = ImageView::Create(imageViewDesc);

    ImageDesc depthImageDesc;
    depthImageDesc.Width = InWidth;
    depthImageDesc.Height = InHeight;
    depthImageDesc.Format = ImageFormat::Depth32F;
    depthImageDesc.Usage = ImageUsage::DepthStencil;
    auto depthImage = Image::Create(depthImageDesc);

    ImageViewDesc depthImageViewDesc;
    depthImageViewDesc.ImagePtr = depthImage;
    depthImageViewDesc.Format = ImageFormat::Depth32F;
    depthImageViewDesc.Aspect = ImageAspect::Depth;
    auto depthImageView = ImageView::Create(depthImageViewDesc);

    ImageDesc idImageDesc;
    idImageDesc.Width = InWidth;
    idImageDesc.Height = InHeight;
    // The id is packed into RGBA8
    idImageDesc.Format = ImageFormat::RGBA8;
    idImageDesc.Usage = ImageUsage::ColorAttachment | ImageUsage::TransferSrc | ImageUsage::Sampled;
    auto idImage = Image::Create(idImageDesc);

    ImageViewDesc idImageViewDesc;
    idImageViewDesc.ImagePtr = idImage;
    idImageViewDesc.Format = ImageFormat::RGBA8;
    auto idImageView = ImageView::Create(idImageViewDesc);

    FrameBufferDesc frameBufferDesc;
    frameBufferDesc.Width = InWidth;
    frameBufferDesc.Height = InHeight;
    frameBufferDesc.Samples = SceneSampleCount;
    frameBufferDesc.ColorAttachments.Add(imageView);
    frameBufferDesc.ColorAttachments.Add(idImageView);
    frameBufferDesc.DepthAttachment = depthImageView;
    frameBufferDesc.ClearColor = Vec4(0.08f, 0.09f, 0.11f, 1.0f);
    frameBufferDesc.ClearColors.Add(Vec4(0.08f, 0.09f, 0.11f, 1.0f));
    frameBufferDesc.ClearColors.Add(Vec4(0.0f));
    m_FrameBuffer = FrameBuffer::Create(frameBufferDesc);

    if (!m_Sampler.Get()) {
        SamplerDesc samplerDesc;
        samplerDesc.MagFilter = FilterMode::Linear;
        samplerDesc.MinFilter = FilterMode::Linear;
        m_Sampler = Sampler::Create(samplerDesc);
    }
    if (!m_UniformBuffer.Get()) {
        m_UniformBuffer = UniformBuffer::Create(0, sizeof(SceneUniformData));
    }
    m_MaterialPipelines.Clear();
}

void ArtifactRenderPipeline::Render(double InDeltaTime, const RenderParams& InParams) {
    if (m_Width != InParams.Width || m_Height != InParams.Height) {
        Invalidate(InParams.Width, InParams.Height);
    }

    UpdateUniformData(InDeltaTime, InParams);

    // Opening the pass here is what clears the target; a material-less world simply draws nothing into it.
    RenderCommandQueue& renderQueue = RenderingAPI::GetInstance()->GetRenderQueue();
    renderQueue.Push(RenderCommandType::BeginRenderPass, CmdBeginRenderPass{ m_FrameBuffer });
    if (InParams.m_World == nullptr) {
        return;
    }

    for (Node* node : InParams.m_World->GetAllNodes()) {
        StaticMeshNode* staticMesh = node->As<StaticMeshNode>();
        Mesh* mesh = staticMesh ? staticMesh->GetMesh() : nullptr;
        if (!mesh || !staticMesh->IsEnabled()) {
            continue;
        }

        Pipeline* pipeline = ResolvePipeline(staticMesh->GetMaterial());
        if (!pipeline) {
            continue;
        }
        renderQueue.Push(RenderCommandType::BindPipeline, CmdBindPipeline{ pipeline });
        renderQueue.Push(RenderCommandType::SetShaderData, CmdSetShaderData{ staticMesh->GetPerMeshShaderData() });

        VertexBuffer* vertexBuffer = mesh->GetVertexBuffer();
        AE_ASSERT(vertexBuffer);
        vertexBuffer->Draw();
    }
}

uint32_t ArtifactRenderPipeline::PickNodeId(uint32_t InX, uint32_t InY) const {
    return m_FrameBuffer.Get() ? m_FrameBuffer->ReadPixelUint(NodeIdAttachment, InX, InY) : 0;
}

SharedObjectPtr<class ImageView> ArtifactRenderPipeline::GetFinalImageView() const {
    if (!m_FrameBuffer.Get()) {
        return nullptr;
    }
    return m_FrameBuffer->GetDesc().ColorAttachments[0];
}

ArtifactRenderPipeline::ArtifactRenderPipeline() {
    Invalidate(100, 100);
}

ArtifactRenderPipeline::~ArtifactRenderPipeline() {

}
