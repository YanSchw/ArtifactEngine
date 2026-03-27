#include "EditorEngine.h"
#include "Core/Log.h"

#include "Window.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/VertexBuffer.h"

SharedObjectPtr<Window> s_Window;


void EditorEngine::Initialize() {
    s_Window = Window::Create(WindowParams{ "Artifact Editor", 1280, 720 });

    Object::Create(Class("VulkanAPI"));
    RenderingAPI::GetInstance()->Initialize();

    Array<Vertex> vertices1 = {
        { { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    VertexBuffer::Create(vertices1, { 0, 1, 2 });
    Array<Vertex> vertices2 = {
        { { -0.5f, -0.5f,  -1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  -1.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  -1.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    VertexBuffer::Create(vertices2, { 0, 1, 2 });
}

bool EditorEngine::MainTick(double InDeltaTime) {
    RenderingAPI::GetInstance()->UpdateUniformData();
    RenderingAPI::GetInstance()->Draw();

    s_Window->PollEvents();
    return !s_Window->ShouldClose();
}

void EditorEngine::Shutdown() {
    s_Window = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}