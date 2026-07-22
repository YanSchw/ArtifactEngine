#include "CoreMinimal.h"
#include "Platform/Platform.h"
#include "Platform/FileIO.h"

#include "ArtifactRenderPipeline.h"
#include "Assets/AssetManager.h"
#include "Assets/Texture2D.h"
#include "Assets/Mesh.h"
#include "Core/EngineConfig.h"
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

static SharedObjectPtr<Shader> s_Shader;

struct SceneUniformData {
    glm::mat4 viewProjectionMatrix;
};

void ArtifactRenderPipeline::UpdateUniformData(const RenderParams& InParams) {
    CameraNode* camera = InParams.CameraOverride;
    if (!camera && InParams.m_World) {
        camera = InParams.m_World->GetMainCamera();
    }

    SceneUniformData data;
    data.viewProjectionMatrix = glm::mat4(1.0f);
    if (camera) {
        camera->SetAspectRatio(InParams.Width / (float) InParams.Height);
        data.viewProjectionMatrix = camera->GetViewProjectionMatrix();
    }

    void* mapped = m_UniformBuffer->MapData(sizeof(data), 0);
    memcpy(mapped, &data, sizeof(data));
    m_UniformBuffer->UnmapData();
}

void ArtifactRenderPipeline::Invalidate(uint32_t InWidth, uint32_t InHeight) {
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
    frameBufferDesc.ColorAttachments.Add(imageView);
    frameBufferDesc.ColorAttachments.Add(idImageView);
    frameBufferDesc.DepthAttachment = depthImageView;
    frameBufferDesc.ClearColor = Vec4(0.08f, 0.09f, 0.11f, 1.0f);
    frameBufferDesc.ClearColors.Add(Vec4(0.08f, 0.09f, 0.11f, 1.0f));
    frameBufferDesc.ClearColors.Add(Vec4(0.0f));
    m_FrameBuffer = FrameBuffer::Create(frameBufferDesc);

    SamplerDesc samplerDesc;
    samplerDesc.MagFilter = FilterMode::Nearest;
    samplerDesc.MinFilter = FilterMode::Nearest;
    auto sampler = Sampler::Create(samplerDesc);
    if (!m_UniformBuffer.Get()) {
        m_UniformBuffer = UniformBuffer::Create(0, sizeof(SceneUniformData));
    }
    PipelineDesc pipelineDesc;
    pipelineDesc.Target = m_FrameBuffer;
    pipelineDesc.Shader = s_Shader;
    pipelineDesc.ClockwiseFrontFace = true;
    pipelineDesc.Buffers.Add(m_UniformBuffer);
    pipelineDesc.ImageBindings.Add({ 16, AssetManager::Get().GetAsset<Texture2D>(UUID::FromString("8c2146d1-c4d7-41b4-b456-9fd071812573"))->GetTexture()->GetDefaultView(), sampler });
    m_Pipeline = Pipeline::Create(pipelineDesc);
}

void ArtifactRenderPipeline::Render(double InDeltaTime, const RenderParams& InParams) {
    if (m_Width != InParams.Width || m_Height != InParams.Height) {
        Invalidate(InParams.Width, InParams.Height);
    }

    UpdateUniformData(InParams);
    m_Pipeline->Bind();
    if (InParams.m_World == nullptr) {
        return;
    }

    for (Node* node : InParams.m_World->GetAllNodes()) {
        if (StaticMeshNode* staticMesh = node->As<StaticMeshNode>()) {
            RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::SetShaderData, CmdSetShaderData{ staticMesh->GetPerMeshShaderData() });

            Mesh* mesh = staticMesh->GetMesh();
            if (staticMesh->IsEnabled() && mesh) {
                VertexBuffer* vertexBuffer = mesh->GetVertexBuffer();
                AE_ASSERT(vertexBuffer);
                vertexBuffer->Draw();
            }
        }
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
    if (!s_Shader.Get()) {
        s_Shader = Shader::Create(FileIO::ReadFileToString(EngineConfig::GetEngineContentDir() + "/Shaders/Shader.glsl"));
    }
    Invalidate(100, 100);
}

ArtifactRenderPipeline::~ArtifactRenderPipeline() {

}
