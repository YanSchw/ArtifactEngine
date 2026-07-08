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
#include "Assets/AssetManager.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/MouseDevice.h"
#include "InputSystem/MouseCodes.h"
#include "Common/UUID.h"

#include "GameFramework/UICanvas.h"
#include "GameFramework/UIImage.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/UIBuilder.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/KeyCodes.h"
#include "Assets/Font.h"
#include "Rendering/UIRenderer.h"

#include <string>

static SharedObjectPtr<Window> s_Window;
static SharedObjectPtr<Pipeline> s_FullScreenPipeline;
static SharedObjectPtr<VertexBuffer> s_FullScreenQuadVertexBuffer;

void EditorEngine::Initialize() {
    s_Window = Window::Create(WindowParams{ "Artifact Editor", 1280, 720 });
    AE_ASSERT(s_Window);

    Object::Create(Platform::GetDefaultRenderingAPIClass());
    AE_ASSERT(RenderingAPI::GetInstance(), "Failed to create RenderingAPI instance!");
    RenderingAPI::GetInstance()->Initialize();

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
    fullscreenDesc.Target = s_Window;
    fullscreenDesc.Shader = Shader::Create(FileIO::ReadFileToString(EngineConfig::GetEngineContentDir() + "/Shaders/Passthrough.glsl"));
    fullscreenDesc.ImageBindings.Add({ 16, m_RenderPipeline->GetFinalImageView(), sampler });
    s_FullScreenPipeline = Pipeline::Create(fullscreenDesc);

    BuildDemoUI();
}

// Demo app-state the declarative UI binds to.
static int s_Count = 0;

void EditorEngine::BuildDemoUI() {
    // Set the default UI font once (see Content/Fonts/Default.asset)
    UINode::SetDefaultFont(AssetManager::Get().GetAsset<Font>(UUID::FromString("f0e1d2c3-b4a5-4967-8899-aabbccddeeff")));

    m_UIRenderer = new UIRenderer();
    m_UICanvas = new UICanvas();
    // Keep the demo UI proportional to the window: authored at 1280x720, scaled with the viewport.
    m_UICanvas->ScaleMode = UICanvasScaleMode::ScaleWithScreenSize;
    m_UICanvas->ReferenceResolution = Vec2(1280.0f, 720.0f);

    UI::VStack(*m_UICanvas, [](UIStack& v) {
        v.Padding = UIPadding(24.0f);
        v.Gap = 8.0f;

        UI::Label(v, [] { return "Count: " + std::to_string(s_Count); });
        UI::Button(v, "Add", [] { s_Count++; }).Fill();

        // Radial progress: sweeps a full circle as the count approaches 10.
        UIImage* progress = v.Add<UIImage>();
        progress->Size = Vec2(32.0f, 32.0f);
        progress->FillMethod = UIImageFill::Radial;
        progress->Tint = Vec4(0.30f, 0.75f, 0.40f, 1.0f);
        progress->Bind = [progress] { progress->FillAmount = (float)(s_Count % 10) / 10.0f; };

        UI::If(v, [] { return s_Count > 3; }, [](UINode& c) {
            UI::Label(c, [] { return String("Big! (> 3)"); });
        });

        // Scrollable list: fixed 24px rows overflow the area and clip; wheel scrolls it.
        UIScrollArea* scroll = v.Add<UIScrollArea>();
        scroll->Fill();
        UI::ForEach(*scroll, [] { return s_Count; }, [](UINode& item, int i) {
            item.Size = { 1.0_rel, 24.0_px };
            UI::Label(item, [i] { return "Item " + std::to_string(i); });
        });
    });
}

void EditorEngine::TickInput(double InDeltaTime) {
    // Refresh devices + evaluate action maps before gameplay reads them.
    InputSystem::Get().Tick((float)InDeltaTime);
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

    // UI pass: composited onto the surface after the scene blit, before the queue is flushed.
    if (m_UIRenderer && m_UICanvas) {
        UIFrameContext uiContext;
        uiContext.DeltaTime = (float)InDeltaTime;
        if (MouseDevice* mouse = MouseDevice::Instance()) {
            uiContext.CursorPosition = mouse->GetPosition();
            uiContext.CursorDown = mouse->IsPressed(MouseCode::Left);
            uiContext.CursorPressedThisFrame = mouse->IsDown(MouseCode::Left);
            uiContext.CursorReleasedThisFrame = mouse->IsUp(MouseCode::Left);
            uiContext.ScrollDelta = mouse->GetScrollDelta();
        }
        if (KeyboardDevice* keyboard = KeyboardDevice::Instance()) {
            uiContext.NavUp = keyboard->IsDown(KeyCode::Up);
            uiContext.NavDown = keyboard->IsDown(KeyCode::Down);
            uiContext.NavLeft = keyboard->IsDown(KeyCode::Left);
            uiContext.NavRight = keyboard->IsDown(KeyCode::Right);
            uiContext.NavSelectPressed = keyboard->IsDown(KeyCode::Enter);
            uiContext.NavSelectReleased = keyboard->IsUp(KeyCode::Enter);
            uiContext.NavBack = keyboard->IsDown(KeyCode::Escape);
        }
        const Vec2 surfaceSize = Vec2((float)s_Window->GetWidth(), (float)s_Window->GetHeight());
        m_UIRenderer->Render(s_Window.Get(), m_UICanvas, surfaceSize, uiContext);
    }

    RenderingAPI::GetInstance()->Draw();

    s_Window->PollEvents();
    return !s_Window->ShouldClose();
}

void EditorEngine::Shutdown() {
    // Tear down the UI (releases its pipelines/buffers) before the surface and RHI go away.
    delete m_UICanvas;
    m_UICanvas = nullptr;
    delete m_UIRenderer;
    m_UIRenderer = nullptr;

    AssetManager::Get().Shutdown();
    s_Window = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}