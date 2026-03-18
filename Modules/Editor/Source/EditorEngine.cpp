#include "EditorEngine.h"
#include "Core/Log.h"

#include "Window.h"
#include "VulkanAPI.h"

SharedObjectPtr<Window> s_Window;


void EditorEngine::Initialize() {
    s_Window = Window::Create(WindowParams{ "Artifact Editor", 1280, 720 });

    VulkanAPI::SetupVulkan();
}

bool EditorEngine::MainTick(double InDeltaTime) {
    VulkanAPI::UpdateUniformData();
    VulkanAPI::Draw();

    s_Window->PollEvents();
    return !s_Window->ShouldClose();
}

void EditorEngine::Shutdown() {
    s_Window = nullptr;
    VulkanAPI::CleanUp(true);
}