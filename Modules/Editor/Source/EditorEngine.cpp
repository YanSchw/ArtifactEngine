#include "EditorEngine.h"
#include "CoreMinimal.h"
#include "Platform/Platform.h"

#include "EditorWindow.h"
#include "Tabs/SceneEditorTab.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Assets/AssetManager.h"
#include "InputSystem/InputSystem.h"
#include "Common/UUID.h"

#include "GameFramework/UICanvas.h"
#include "Assets/Font.h"

void EditorEngine::Initialize() {
    SharedObjectPtr<EditorWindow> window = EditorWindow::Create(WindowParams{ "Artifact Editor", 1280, 720 });
    AE_ASSERT(window);

    Object::Create(Platform::GetDefaultRenderingAPIClass());
    AE_ASSERT(RenderingAPI::GetInstance(), "Failed to create RenderingAPI instance!");
    RenderingAPI::GetInstance()->Initialize();

    (new AssetManager())->Initialize();

    m_RenderPipeline = Object::Create(EngineConfig::RenderPipelineClass())->As<RenderPipeline>();
    AE_ASSERT(m_RenderPipeline, "Failed to create RenderPipeline instance!");

    // Set the default UI font once (see Content/Fonts/Default.asset)
    UINode::SetDefaultFont(AssetManager::Get().GetAsset<Font>(UUID::FromString("f0e1d2c3-b4a5-4967-8899-aabbccddeeff")));

    window->OpenTab<SceneEditorTab>();
    window->OpenTab<SceneEditorTab>();
    window->OpenTab<SceneEditorTab>();

    // Keep rendering while a modal resize/move blocks the main loop's event pump.
    Window::SetRefreshCallback([this]() { RenderFrame(m_DeltaTime); });
}

void EditorEngine::TickInput(double InDeltaTime) {
    Window::PollEvents();
    // Refresh devices + evaluate action maps before gameplay reads them.
    InputSystem::Get().Tick((float)InDeltaTime);
}

void EditorEngine::RenderFrame(double InDeltaTime) {
    Window* sceneWindow = nullptr;
    for (const SharedObjectPtr<ThemedWindow>& window : ThemedWindow::GetAllWindows()) {
        if (window->IsA<EditorWindow>()) {
            sceneWindow = window.Get();
            break;
        }
    }
    if (!sceneWindow) {
        return;
    }

    m_RenderPipeline->Render(InDeltaTime, RenderParams{ sceneWindow->GetWidth(), sceneWindow->GetHeight() });

    for (int32_t i = 0; i < ThemedWindow::GetAllWindows().Size(); i++) {
        ThemedWindow::GetAllWindows()[i]->RenderWindow(InDeltaTime);
    }

    RenderingAPI::GetInstance()->Draw();
}

bool EditorEngine::MainTick(double InDeltaTime) {
    RenderFrame(InDeltaTime);

    // Destroying a window can cascade, so restart the sweep whenever one goes away.
    bool sweptWindow = true;
    while (sweptWindow) {
        sweptWindow = false;
        for (int32_t i = 0; i < ThemedWindow::GetAllWindows().Size(); i++) {
            ThemedWindow* window = ThemedWindow::GetAllWindows()[i].Get();
            if (window && window->ShouldClose() && window->OnCloseRequested()) {
                ThemedWindow::DestroyWindow(window);
                sweptWindow = true;
                break;
            }
        }
    }

    for (const SharedObjectPtr<ThemedWindow>& window : ThemedWindow::GetAllWindows()) {
        if (window->IsA<EditorWindow>()) {
            return true;
        }
    }
    return false;
}

void EditorEngine::Shutdown() {
    // Tear the windows (and their UI renderers) down before the assets they sample and the RHI.
    ThemedWindow::DestroyAllWindows();
    AssetManager::Get().Shutdown();
    RenderingAPI::GetInstance()->CleanUp(true);
}
