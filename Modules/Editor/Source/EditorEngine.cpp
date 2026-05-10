#include "EditorEngine.h"
#include "CoreMinimal.h"
#include "Platform/Platform.h"
#include "Platform/FileIO.h"

#include "Window.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderPipeline.h"
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
static SharedObjectPtr<Pipeline> s_FullScreenPipeline;
static SharedObjectPtr<VertexBuffer> s_FullScreenQuadVertexBuffer;

void EditorEngine::Initialize() {
    s_Window = Window::Create(WindowParams{ "Artifact Editor", 1280, 720 });
    AE_ASSERT(s_Window);

    Object::Create(Platform::GetDefaultRenderingAPIClass());
    AE_ASSERT(RenderingAPI::GetInstance(), "Failed to create RenderingAPI instance!");
    RenderingAPI::GetInstance()->Initialize();

    m_RenderPipeline = Object::Create(EngineConfig::RenderPipelineClass())->As<RenderPipeline>();
    AE_ASSERT(m_RenderPipeline, "Failed to create RenderPipeline instance!");

    Array<Vertex> fullScreenQuadVertices = {
        { { -1.0f, -1.0f,  0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f,  0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f,  0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }
    };
    s_FullScreenQuadVertexBuffer = VertexBuffer::Create(fullScreenQuadVertices, { 0, 1, 2, 0, 2, 3 });

    auto sampler = Sampler::Create({ FilterMode::Nearest, FilterMode::Nearest, AddressMode::Repeat, AddressMode::Repeat, AddressMode::Repeat });

    PipelineDesc fullscreenDesc;
    fullscreenDesc.Target = s_Window;
    fullscreenDesc.Shader = Shader::Create(FileIO::ReadFileToString(EngineConfig::ContentDir() + "/Shaders/Passthrough.glsl"));
    fullscreenDesc.ImageBindings.Add({ 16, m_RenderPipeline->GetFinalImageView(), sampler });
    s_FullScreenPipeline = Pipeline::Create(fullscreenDesc);
}

bool EditorEngine::MainTick(double InDeltaTime) {
    m_RenderPipeline->Render(InDeltaTime, RenderParams{ s_Window->GetWidth(), s_Window->GetHeight() });

    auto[binding, imageView, sampler] = s_FullScreenPipeline->GetDesc().ImageBindings[0];
    if (imageView != m_RenderPipeline->GetFinalImageView()) {
        PipelineDesc fullscreenDesc = s_FullScreenPipeline->GetDesc();
        fullscreenDesc.ImageBindings[0] = { 16, m_RenderPipeline->GetFinalImageView(), sampler };
        s_FullScreenPipeline = Pipeline::Create(fullscreenDesc);
    }

    s_FullScreenPipeline->Bind();
    s_FullScreenQuadVertexBuffer->Draw();

    RenderingAPI::GetInstance()->Draw();

    s_Window->PollEvents();
    return !s_Window->ShouldClose();
}

void EditorEngine::Shutdown() {
    s_Window = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}