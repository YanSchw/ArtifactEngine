#include "GameEngine.h"
#include "CoreMinimal.h"
#include "Platform/Platform.h"
#include "Platform/FileIO.h"

#include "Window.h"
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

static SharedObjectPtr<Window> s_Window;

static SharedObjectPtr<VertexBuffer> s_VertexBuffer1;
static SharedObjectPtr<VertexBuffer> s_VertexBuffer2;

static SharedObjectPtr<Shader> s_Shader;
static SharedObjectPtr<Texture> s_Texture;
static SharedObjectPtr<UniformBuffer> s_UniformBuffer;
static SharedObjectPtr<Pipeline> s_Pipeline;
static SharedObjectPtr<FrameBuffer> s_FrameBuffer;


static SharedObjectPtr<Pipeline> s_FullScreenPipeline;
static SharedObjectPtr<VertexBuffer> s_FullScreenQuadVertexBuffer;

static struct {
    glm::mat4 transformationMatrix;
} uniformBufferData;

static void UpdateUniformData() {
    static auto timeStart = std::chrono::high_resolution_clock::now(); // Initialize once

    auto timeNow = std::chrono::high_resolution_clock::now();
    long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - timeStart).count();
    float angle = (millis % 4000) / 4000.0f * glm::radians(360.f);

    // Proper identity initialization
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0, 0, 1));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f / 3.0f, -0.5f / 3.0f, 0.0f));

    // View and projection
    auto viewMatrix = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
    auto projMatrix = glm::perspective(glm::radians(70.f), s_Window->GetWidth() / (float)s_Window->GetHeight(), 0.1f, 10.0f);

    uniformBufferData.transformationMatrix = projMatrix * viewMatrix * modelMatrix;

    // Upload
    void* data = s_UniformBuffer->MapData(sizeof(uniformBufferData), 0);
    memcpy(data, &uniformBufferData, sizeof(uniformBufferData));
    s_UniformBuffer->UnmapData();
}

void GameEngine::Initialize() {
    s_Window = Window::Create(WindowParams{ "Artifact Engine", 1280, 720 });
    AE_ASSERT(s_Window);

    Object::Create(Platform::GetDefaultRenderingAPIClass());
    AE_ASSERT(RenderingAPI::GetInstance(), "Failed to create RenderingAPI instance!");
    RenderingAPI::GetInstance()->Initialize();

    Array<Vertex> vertices1 = {
        { { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }
    };
    s_VertexBuffer1 = VertexBuffer::Create(vertices1, { 0, 1, 2 });
    Array<Vertex> vertices2 = {
        { { -0.5f, -0.5f,  -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  -1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { {  0.5f,  0.5f,  -1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }
    };
    s_VertexBuffer2 = VertexBuffer::Create(vertices2, { 0, 1, 2 });
    Array<Vertex> fullScreenQuadVertices = {
        { { -1.0f, -1.0f,  0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f,  0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f,  0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }
    };
    s_FullScreenQuadVertexBuffer = VertexBuffer::Create(fullScreenQuadVertices, { 0, 1, 2, 0, 2, 3 });


    ImageDesc imageDesc;
    imageDesc.Width = s_Window->GetWidth();
    imageDesc.Height = s_Window->GetHeight();
    imageDesc.Format = ImageFormat::RGBA8;
    imageDesc.Usage = ImageUsage::ColorAttachment | ImageUsage::Sampled;
    auto image = Image::Create(imageDesc);
    ImageViewDesc imageViewDesc;
    imageViewDesc.Image = image;
    imageViewDesc.Format = ImageFormat::RGBA8;
    auto imageView = ImageView::Create(imageViewDesc);

    ImageDesc depthImageDesc;
    depthImageDesc.Width = s_Window->GetWidth();
    depthImageDesc.Height = s_Window->GetHeight();
    depthImageDesc.Format = ImageFormat::Depth32F;
    depthImageDesc.Usage = ImageUsage::DepthStencil;
    auto depthImage = Image::Create(depthImageDesc);

    ImageViewDesc depthImageViewDesc;
    depthImageViewDesc.Image = depthImage;
    depthImageViewDesc.Format = ImageFormat::Depth32F;
    auto depthImageView = ImageView::Create(depthImageViewDesc);

    FrameBufferDesc frameBufferDesc;
    frameBufferDesc.Width = s_Window->GetWidth();
    frameBufferDesc.Height = s_Window->GetHeight();
    frameBufferDesc.ColorAttachments.Add(imageView);
    frameBufferDesc.DepthAttachment = depthImageView;
    s_FrameBuffer = FrameBuffer::Create(frameBufferDesc);

    s_Shader = Shader::Create(FileIO::ReadFileToString(EngineConfig::ContentDir() + "/Shaders/Shader.glsl"));
    s_Texture = Texture::Create(EngineConfig::ContentDir() + "/Textures/Checkerboard.png");
    SamplerDesc samplerDesc;
    samplerDesc.MagFilter = FilterMode::Nearest;
    samplerDesc.MinFilter = FilterMode::Nearest;
    auto sampler = Sampler::Create(samplerDesc);
    s_UniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
    PipelineDesc pipelineDesc;
    pipelineDesc.Target = s_FrameBuffer;
    pipelineDesc.Shader = s_Shader;
    pipelineDesc.Buffers.Add(s_UniformBuffer);
    pipelineDesc.ImageBindings.Add({ 16, s_Texture->GetDefaultView(), sampler });
    s_Pipeline = Pipeline::Create(pipelineDesc);


    PipelineDesc fullscreenDesc;
    fullscreenDesc.Target = s_Window;
    fullscreenDesc.Shader = Shader::Create(FileIO::ReadFileToString(EngineConfig::ContentDir() + "/Shaders/Passthrough.glsl"));
    fullscreenDesc.ImageBindings.Add({ 16, imageView, sampler });
    s_FullScreenPipeline = Pipeline::Create(fullscreenDesc);
}

bool GameEngine::MainTick(double InDeltaTime) {
    UpdateUniformData();
    s_Pipeline->Bind();
    s_VertexBuffer1->Draw();
    s_VertexBuffer2->Draw();

    s_FullScreenPipeline->Bind();
    s_FullScreenQuadVertexBuffer->Draw();

    RenderingAPI::GetInstance()->Draw();

    s_Window->PollEvents();
    return !s_Window->ShouldClose();
}

void GameEngine::Shutdown() {
    s_Window = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}