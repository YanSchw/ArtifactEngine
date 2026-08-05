#include "EditorEngine.h"
#include "CoreMinimal.h"
#include "Platform/Platform.h"
#include "Platform/PlatformHooks.h"

#include "EditorWindow.h"
#include "Tabs/SceneEditorTab.h"
#include "Rendering/RenderingAPI.h"
#include "Assets/AssetManager.h"
#include "Assets/Scene.h"
#include "Core/EngineConfig.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/MouseCodes.h"
#include "Common/UUID.h"

#include "GameFramework/GameInstance.h"
#include "GameFramework/UICanvas.h"
#include "Assets/Font.h"

static bool AnyCursorLocked() {
    for (const SharedObjectPtr<ThemedWindow>& window : ThemedWindow::GetAllWindows()) {
        if (window.Get() && window->IsCursorLocked()) {
            return true;
        }
    }
    Window* primary = Window::GetInstance();
    return primary && primary->IsCursorLocked();
}

static void UnlockAllCursors() {
    for (const SharedObjectPtr<ThemedWindow>& window : ThemedWindow::GetAllWindows()) {
        if (window.Get() && window->IsCursorLocked()) {
            window->SetCursorLocked(false);
        }
    }
    if (Window* primary = Window::GetInstance()) {
        if (primary->IsCursorLocked()) {
            primary->SetCursorLocked(false);
        }
    }
}

static bool EditorIsDraggingCursor() {
    Window* focused = Window::GetFocusedWindow();
    return focused && (focused->IsMouseButtonDown((int32_t)MouseCode::Right)
                    || focused->IsMouseButtonDown((int32_t)MouseCode::Middle));
}

EditorEngine* EditorEngine::Get() {
    return Cast<EditorEngine>(&Engine::Get());
}

GameInstance* EditorEngine::CreatePlayInstance(SceneEditorTab* InTab) {
    if (SceneEditorTab* previous = m_PlayingTab.Get()) {
        if (previous != InTab) {
            previous->StopPlayInEditor();
        }
    }
    m_PlayingTab = InTab;
    m_GameCursorLocked = false;
    m_HadGameInput = false;
    m_GameInstance = new GameInstance();
    return m_GameInstance.Get();
}

void EditorEngine::DisposePlayInstance() {
    if (m_GameCursorLocked) {
        UnlockAllCursors();
        m_GameCursorLocked = false;
    }
    m_HadGameInput = false;
    m_PlayingTab = nullptr;
    m_GameInstance = nullptr;
}

void EditorEngine::UpdatePlayCursor() {
    const bool gameInput = IsGameInputActive();
    if (gameInput && !m_HadGameInput) {
        SceneEditorTab* tab = m_GameCursorLocked ? m_PlayingTab.Get() : nullptr;
        if (EditorWindow* window = tab ? tab->GetOwnerWindow() : nullptr) {
            window->SetCursorLocked(true);
        }
    } else if (gameInput) {
        m_GameCursorLocked = AnyCursorLocked();
    } else if (AnyCursorLocked() && !EditorIsDraggingCursor()) {
        m_GameCursorLocked = true;
        UnlockAllCursors();
    }
    m_HadGameInput = gameInput;
}

bool EditorEngine::IsGameInputActive() const {
    SceneEditorTab* tab = m_PlayingTab.Get();
    if (!tab || tab->GetPlayState() != PlayState::Playing) {
        return false;
    }
    EditorWindow* window = tab->GetOwnerWindow();
    return window && window->GetActiveTab() == tab && window->IsFocused();
}

void EditorEngine::Initialize() {
    SharedObjectPtr<EditorWindow> window = EditorWindow::Create(WindowParams{ "Artifact Editor", 1280, 720 });
    AE_ASSERT(window);

    Object::Create(Platform::GetDefaultRenderingAPIClass());
    AE_ASSERT(RenderingAPI::GetInstance(), "Failed to create RenderingAPI instance!");
    RenderingAPI::GetInstance()->Initialize();

    (new AssetManager())->Initialize();

    // Set the default UI font once (see Content/Fonts/Default.asset)
    UINode::SetDefaultFont(AssetManager::Get().GetAsset<Font>(UUID::FromString("f0e1d2c3-b4a5-4967-8899-aabbccddeeff")));

    Scene* scene = AssetManager::Get().GetAsset<Scene>(EngineConfig::GetConfigVar<UUID>("DefaultScene"));
    if (!scene) {
        Array<Asset*> scenes = AssetManager::Get().GetAssetsOfClass(Scene::StaticClass());
        scene = scenes.IsEmpty() ? nullptr : Cast<Scene>(scenes[0]);
    }
    window->OpenTab<SceneEditorTab>()->OpenScene(scene);

    // Keep rendering while a modal resize/move blocks the main loop's event pump.
    Window::SetRefreshCallback([this]() { RenderFrame(m_DeltaTime); });
}

void EditorEngine::TickInput(double InDeltaTime) {
    Window::PollEvents();
    PlatformHooks::Get().SetSystemShortcutsSuppressed(Window::GetFocusedWindow() != nullptr);
    InputSystem::Get().SetActionsSuppressed(!IsGameInputActive());
    // Refresh devices + evaluate action maps before gameplay reads them.
    InputSystem::Get().Tick((float)InDeltaTime);
}

void EditorEngine::RenderFrame(double InDeltaTime) {
    if (ThemedWindow::GetAllWindows().IsEmpty()) {
        return;
    }
    for (int32_t i = 0; i < ThemedWindow::GetAllWindows().Size(); i++) {
        ThemedWindow::GetAllWindows()[i]->RenderWindow(InDeltaTime);
    }

    RenderingAPI::GetInstance()->Draw();
}

bool EditorEngine::MainTick(double InDeltaTime) {
    if (GameInstance* game = GetGameInstance()) {
        game->Update(InDeltaTime);
        UpdatePlayCursor();
    }

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
    PlatformHooks::Get().SetSystemShortcutsSuppressed(false);
    DisposePlayInstance();
    // Tear the windows (and their UI renderers) down before the assets they sample and the RHI.
    ThemedWindow::DestroyAllWindows();
    AssetManager::Get().Shutdown();
    RenderingAPI::GetInstance()->CleanUp(true);
}
