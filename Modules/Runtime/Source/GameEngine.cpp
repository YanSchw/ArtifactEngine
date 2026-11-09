#include "GameEngine.h"
#include "CoreMinimal.h"
#include "Platform/Platform.h"

#include "Rendering/Surface.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/MouseDevice.h"
#include "GameFramework/UINode.h"
#include "GameFramework/UIWorld.h"
#include "Assets/Font.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/ShaderLibrary.h"
#include "Assets/AssetManager.h"
#include "Audio/AudioEngine.h"
#include "Assets/Scene.h"
#include "Common/UUID.h"
#include "GameFramework/GameInstance.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "CameraController.h"
#include "GameFramework/StaticMeshNode.h"

static SharedObjectPtr<Surface> s_Surface;

static bool IsSurfaceDrawable() {
    return s_Surface && !s_Surface->IsMinimized() && s_Surface->GetWidth() > 0 && s_Surface->GetHeight() > 0;
}

void GameEngine::Initialize() {
    s_Surface = Surface::CreateMain(SurfaceParams{ "Artifact Engine", 1280, 720,
                                                   EngineConfig::GetConfigVar<bool>("Fullscreen") });
    AE_ASSERT(s_Surface);

    Object::Create(Platform::GetDefaultRenderingAPIClass());
    AE_ASSERT(RenderingAPI::GetInstance(), "Failed to create RenderingAPI instance!");
    RenderingAPI::GetInstance()->Initialize();
    ShaderLibrary::Initialize();

    (new AssetManager())->Initialize();
    (new AudioEngine())->Initialize();

    m_RenderPipeline = RenderPipeline::Create();

    if (Font* font = AssetManager::Get().GetAsset<Font>(EngineConfig::GetConfigVar<UUID>("DefaultFont"))) {
        AssetManager::Get().LoadAsset(font);
        UINode::SetDefaultFont(font);
    }

    // create GameInstance and World
    m_GameInstance = new GameInstance();
    World* world = m_GameInstance->CreateNewWorld(true);

    const UUID defaultSceneId = EngineConfig::GetConfigVar<UUID>("DefaultScene");
    if (Scene* defaultScene = AssetManager::Get().GetAsset<Scene>(defaultSceneId)) {
        world->Populate(defaultScene);
    } else if (defaultSceneId.IsValid()) {
        AE_ERROR("DefaultScene {0} could not be found", defaultSceneId.ToString());
    }

    s_Surface->SetRedrawCallback([this]() {
        UIFrameContext context;
        context.CursorPosition = Vec2(-1.0e6f);
        RouteUI(context);
        RenderFrame(m_DeltaTime);
    });
}

void GameEngine::TickInput(double InDeltaTime) {
    s_Surface->ProcessEvents();
    // Refresh devices + evaluate action maps before gameplay reads them.
    InputSystem::Get().Tick((float)InDeltaTime);
}

UIFrameContext GameEngine::ReadUIInput(double InDeltaTime) const {
    UIFrameContext context;
    context.DeltaTime = (float)InDeltaTime;
    context.TextInput = s_Surface->ConsumeTextInput();
    if (MouseDevice* mouse = MouseDevice::Instance()) {
        context.CursorPosition = mouse->GetPosition();
        context.CursorDown = mouse->IsPressed(MouseCode::Left);
        context.CursorPressedThisFrame = mouse->IsDown(MouseCode::Left);
        context.CursorReleasedThisFrame = mouse->IsUp(MouseCode::Left);
        context.SecondaryDown = mouse->IsPressed(MouseCode::Right);
        context.SecondaryPressedThisFrame = mouse->IsDown(MouseCode::Right);
        context.SecondaryReleasedThisFrame = mouse->IsUp(MouseCode::Right);
        context.ScrollDelta = mouse->GetScrollDelta();
    }
    if (KeyboardDevice* keyboard = KeyboardDevice::Instance()) {
        context.NavUp = keyboard->IsDown(KeyCode::Up);
        context.NavDown = keyboard->IsDown(KeyCode::Down);
        context.NavLeft = keyboard->IsDown(KeyCode::Left);
        context.NavRight = keyboard->IsDown(KeyCode::Right);
        context.NavSelectPressed = keyboard->IsDown(KeyCode::Enter);
        context.NavSelectReleased = keyboard->IsUp(KeyCode::Enter);
        context.NavBack = keyboard->IsDown(KeyCode::Escape);
    }
    return context;
}

void GameEngine::RouteUI(const UIFrameContext& InContext) {
    UIWorld* ui = UIWorld::Get(GetGameInstance()->GetCurrentWorld());
    if (ui && IsSurfaceDrawable()) {
        ui->Route(Vec2((float)s_Surface->GetWidth(), (float)s_Surface->GetHeight()), InContext);
    }
}

void GameEngine::RenderFrame(double InDeltaTime) {
    if (!IsSurfaceDrawable()) {
        return;
    }

    RenderParams params;
    params.Width = s_Surface->GetWidth();
    params.Height = s_Surface->GetHeight();
    params.m_World = GetGameInstance()->GetCurrentWorld();
    params.PresentTarget = s_Surface;
    m_RenderPipeline->Render(InDeltaTime, params);

    RenderingAPI::GetInstance()->Draw();
}

bool GameEngine::MainTick(double InDeltaTime) {
    GetGameInstance()->Update(InDeltaTime);
    RouteUI(ReadUIInput(InDeltaTime));
    RenderFrame(InDeltaTime);
    return !s_Surface->ShouldClose();
}

void GameEngine::Shutdown() {
    m_GameInstance = nullptr;
    UINode::SetDefaultFont(nullptr);
    AudioEngine::Get().Shutdown();
    AssetManager::Get().Shutdown();
    ShaderLibrary::Shutdown();
    s_Surface = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}
