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
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/StaticMeshNode.h"

static SharedObjectPtr<Shader> s_Shader;
static SharedObjectPtr<UniformBuffer> s_UniformBuffer;
static SharedObjectPtr<Pipeline> s_Pipeline;
static SharedObjectPtr<FrameBuffer> s_FrameBuffer;

static SharedObjectPtr<Pipeline> s_FullScreenPipeline;
static SharedObjectPtr<VertexBuffer> s_FullScreenQuadVertexBuffer;

static uint32_t s_Width  = 0;
static uint32_t s_Height = 0;

static struct {
    glm::mat4 viewProjectionMatrix;
} uniformBufferData;

static void UpdateUniformData(const RenderParams& InParams) {
    if (InParams.m_World && InParams.m_World->GetMainCamera()) {
        InParams.m_World->GetMainCamera()->SetAspectRatio(InParams.Width / (float) InParams.Height);
        uniformBufferData.viewProjectionMatrix = InParams.m_World->GetMainCamera()->GetViewProjectionMatrix();
    }

    // Upload
    void* data = s_UniformBuffer->MapData(sizeof(uniformBufferData), 0);
    memcpy(data, &uniformBufferData, sizeof(uniformBufferData));
    s_UniformBuffer->UnmapData();
}

void ArtifactRenderPipeline::Invalidate(uint32_t InWidth, uint32_t InHeight) {
    s_Width = InWidth;
    s_Height = InHeight;

    Array<Vertex> fullScreenQuadVertices = {
        { { -1.0f, -1.0f,  0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f,  0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f,  0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }
    };
    s_FullScreenQuadVertexBuffer = VertexBuffer::Create(fullScreenQuadVertices, { 0, 1, 2, 0, 2, 3 });


    ImageDesc imageDesc;
    imageDesc.Width = InWidth;
    imageDesc.Height = InHeight;
    imageDesc.Format = ImageFormat::RGBA8;
    imageDesc.Usage = ImageUsage::ColorAttachment | ImageUsage::Sampled;
    auto image = Image::Create(imageDesc);
    ImageViewDesc imageViewDesc;
    imageViewDesc.Image = image;
    imageViewDesc.Format = ImageFormat::RGBA8;
    auto imageView = ImageView::Create(imageViewDesc);

    ImageDesc depthImageDesc;
    depthImageDesc.Width = InWidth;
    depthImageDesc.Height = InHeight;
    depthImageDesc.Format = ImageFormat::Depth32F;
    depthImageDesc.Usage = ImageUsage::DepthStencil;
    auto depthImage = Image::Create(depthImageDesc);

    ImageViewDesc depthImageViewDesc;
    depthImageViewDesc.Image = depthImage;
    depthImageViewDesc.Format = ImageFormat::Depth32F;
    auto depthImageView = ImageView::Create(depthImageViewDesc);

    FrameBufferDesc frameBufferDesc;
    frameBufferDesc.Width = InWidth;
    frameBufferDesc.Height = InHeight;
    frameBufferDesc.ColorAttachments.Add(imageView);
    frameBufferDesc.DepthAttachment = depthImageView;
    s_FrameBuffer = FrameBuffer::Create(frameBufferDesc);

    s_Shader = Shader::Create(FileIO::ReadFileToString(EngineConfig::ContentDir() + "/Shaders/Shader.glsl"));
    SamplerDesc samplerDesc;
    samplerDesc.MagFilter = FilterMode::Nearest;
    samplerDesc.MinFilter = FilterMode::Nearest;
    auto sampler = Sampler::Create(samplerDesc);
    s_UniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
    PipelineDesc pipelineDesc;
    pipelineDesc.Target = s_FrameBuffer;
    pipelineDesc.Shader = s_Shader;
    pipelineDesc.Buffers.Add(s_UniformBuffer);
    pipelineDesc.ImageBindings.Add({ 16, AssetManager::Get().GetAsset<Texture2D>(UUID::FromString("8c2146d1-c4d7-41b4-b456-9fd071812573"))->GetTexture()->GetDefaultView(), sampler });
    s_Pipeline = Pipeline::Create(pipelineDesc);
}

void ArtifactRenderPipeline::Render(double InDeltaTime, const RenderParams& InParams) {
    if (s_Width != InParams.Width || s_Height != InParams.Height) {
        Invalidate(InParams.Width, InParams.Height);
    }

    if (InParams.m_World == nullptr) {
        AE_WARN("ArtifactRenderPipeline cannot be invoked without a world");
        return;
    }

    UpdateUniformData(InParams);
    s_Pipeline->Bind();
    for (Node* node : InParams.m_World->GetAllNodes()) {
        if (StaticMeshNode* staticMesh = node->As<StaticMeshNode>()) {
            RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::SetShaderData, CmdSetShaderData{ staticMesh->GetPerMeshShaderData() });

            Mesh* mesh = staticMesh->GetMesh();
            if (mesh) {
                VertexBuffer* vertexBuffer = mesh->GetVertexBuffer();
                AE_ASSERT(vertexBuffer);
                vertexBuffer->Draw();
            }
        }
    }
}

SharedObjectPtr<class ImageView> ArtifactRenderPipeline::GetFinalImageView() const {
    if (!s_FrameBuffer) {
        return nullptr;
    }
    return s_FrameBuffer->GetDesc().ColorAttachments[0];
}

ArtifactRenderPipeline::ArtifactRenderPipeline() {
    Invalidate(100, 100);
}

ArtifactRenderPipeline::~ArtifactRenderPipeline() {

}