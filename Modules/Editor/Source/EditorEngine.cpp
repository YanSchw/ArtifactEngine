#include "EditorEngine.h"
#include "CoreMinimal.h"
#include "Core/Platform.h"
#include "Platform/FileIO.h"

#include "Window.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/Pipeline.h"

SharedObjectPtr<Window> s_Window;

SharedObjectPtr<VertexBuffer> s_VertexBuffer1;
SharedObjectPtr<VertexBuffer> s_VertexBuffer2;

SharedObjectPtr<Shader> s_Shader;
SharedObjectPtr<Pipeline> s_Pipeline;

void EditorEngine::Initialize() {
    s_Window = Window::Create(WindowParams{ "Artifact Editor", 1280, 720 });
    AE_ASSERT(s_Window);

    Object::Create(Platform::GetDefaultRenderingAPIClass());
    AE_ASSERT(RenderingAPI::GetInstance(), "Failed to create RenderingAPI instance!");
    RenderingAPI::GetInstance()->Initialize();

    Array<Vertex> vertices1 = {
        { { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    s_VertexBuffer1 = VertexBuffer::Create(vertices1, { 0, 1, 2 });
    Array<Vertex> vertices2 = {
        { { -0.5f, -0.5f,  -1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  -1.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  -1.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    s_VertexBuffer2 = VertexBuffer::Create(vertices2, { 0, 1, 2 });

    s_Shader = Shader::Create(FileIO::ReadFileToString("/Users/yannick/Developer/ArtifactEngine/Content/Shaders/Shader.glsl"));
    PipelineDesc pipelineDesc;
    pipelineDesc.Shader = s_Shader;
    s_Pipeline = Pipeline::Create(pipelineDesc);
}

bool EditorEngine::MainTick(double InDeltaTime) {
    RenderingAPI::GetInstance()->UpdateUniformData();
    s_Pipeline->Bind();
    s_VertexBuffer1->Draw();
    s_VertexBuffer2->Draw();

    RenderingAPI::GetInstance()->Draw();

    s_Window->PollEvents();
    return !s_Window->ShouldClose();
}

void EditorEngine::Shutdown() {
    s_Window = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}