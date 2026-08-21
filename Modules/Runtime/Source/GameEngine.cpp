#include "GameEngine.h"
#include "CoreMinimal.h"
#include "Platform/Platform.h"

#include "Rendering/Surface.h"
#include "InputSystem/InputSystem.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/Texture.h"
#include "Rendering/Sampler.h"
#include "Rendering/Buffer.h"
#include "Rendering/Pipeline.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Image.h"
#include "Assets/AssetManager.h"
#include "Assets/Scene.h"
#include "Common/UUID.h"
#include "GameFramework/DebugDraw.h"
#include "GameFramework/GameInstance.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "CameraController.h"
#include "GameFramework/StaticMeshNode.h"

static SharedObjectPtr<Surface> s_Surface;
static SharedObjectPtr<Pipeline> s_FullScreenPipeline;
static SharedObjectPtr<VertexBuffer> s_FullScreenQuadVertexBuffer;

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
    fullscreenDesc.Target = s_Surface;
    fullscreenDesc.Shader = ShaderLibrary::CreateShader("/Shaders/Passthrough.glsl");
    fullscreenDesc.ImageBindings.Add({ 16, m_RenderPipeline->GetFinalImageView(), sampler });
    s_FullScreenPipeline = Pipeline::Create(fullscreenDesc);

    // create GameInstance and World
    m_GameInstance = new GameInstance();
    World* world = m_GameInstance->CreateNewWorld(true);

    const UUID defaultSceneId = EngineConfig::GetConfigVar<UUID>("DefaultScene");
    if (Scene* defaultScene = AssetManager::Get().GetAsset<Scene>(defaultSceneId)) {
        world->Populate(defaultScene);
    } else if (defaultSceneId.IsValid()) {
        AE_ERROR("DefaultScene {0} could not be found", defaultSceneId.ToString());
    }

    s_Surface->SetRedrawCallback([this]() { RenderFrame(m_DeltaTime); });
}

void GameEngine::TickInput(double InDeltaTime) {
    s_Surface->ProcessEvents();
    // Refresh devices + evaluate action maps before gameplay reads them.
    InputSystem::Get().Tick((float)InDeltaTime);
}

void GameEngine::RenderFrame(double InDeltaTime) {
    if (!IsSurfaceDrawable()) {
        return;
    }

    World* world = GetGameInstance()->GetCurrentWorld();
    m_RenderPipeline->Render(InDeltaTime, RenderParams {
        s_Surface->GetWidth(),
        s_Surface->GetHeight(),
        world
    });

    DebugDraw::Render(world, m_RenderPipeline->GetFrameBuffer().Get(),
                      world ? world->GetMainCamera() : nullptr);

    auto[binding, imageView, sampler] = s_FullScreenPipeline->GetDesc().ImageBindings[0];
    if (imageView != m_RenderPipeline->GetFinalImageView()) {
        PipelineDesc fullscreenDesc = s_FullScreenPipeline->GetDesc();
        fullscreenDesc.ImageBindings[0] = { 16, m_RenderPipeline->GetFinalImageView(), sampler };
        s_FullScreenPipeline = Pipeline::Create(fullscreenDesc);
    }

    s_FullScreenPipeline->Bind();
    s_FullScreenQuadVertexBuffer->Draw();

    RenderingAPI::GetInstance()->Draw();
}

bool GameEngine::MainTick(double InDeltaTime) {
    GetGameInstance()->Update(InDeltaTime);
    RenderFrame(InDeltaTime);
    return !s_Surface->ShouldClose();
}

void GameEngine::Shutdown() {
    m_GameInstance = nullptr;
    AssetManager::Get().Shutdown();
    ShaderLibrary::Shutdown();
    s_Surface = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}