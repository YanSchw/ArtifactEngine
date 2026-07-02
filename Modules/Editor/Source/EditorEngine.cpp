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

#include "GameFramework/UINode.h"
#include "GameFramework/UIButton.h"
#include "Assets/Font.h"
#include "Rendering/UIRenderer.h"

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
    fullscreenDesc.Shader = Shader::Create(FileIO::ReadFileToString(EngineConfig::ContentDir() + "/Shaders/Passthrough.glsl"));
    fullscreenDesc.ImageBindings.Add({ 16, m_RenderPipeline->GetFinalImageView(), sampler });
    s_FullScreenPipeline = Pipeline::Create(fullscreenDesc);

    BuildDemoUI();
}

void EditorEngine::BuildDemoUI() {
    // Set the default UI font once (see Content/Fonts/Default.asset)
    UINode::SetDefaultFont(AssetManager::Get().GetAsset<Font>(UUID::FromString("f0e1d2c3-b4a5-4967-8899-aabbccddeeff")));

    m_UIRenderer = new UIRenderer();
    m_UIRoot = (new UINode())->Fill();

    UIButton* button = m_UIRoot->Add<UIButton>();
    button->Center({ 220.0f, 56.0f });
    button->SetCaption("Click Me 0");
    button->OnClick = [] { AE_INFO("UI Button clicked!"); };
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

    // UI overlay: composited onto the surface after the scene blit, before the queue is flushed.
    if (m_UIRenderer && m_UIRoot) {
        UIFrameContext uiContext;
        uiContext.DeltaTime = (float)InDeltaTime;
        if (MouseDevice* mouse = MouseDevice::Instance()) {
            uiContext.MousePosition = mouse->GetPosition();
            uiContext.MouseDown = mouse->IsPressed(MouseCode::Left);
            uiContext.MousePressedThisFrame = mouse->IsDown(MouseCode::Left);
            uiContext.MouseReleasedThisFrame = mouse->IsUp(MouseCode::Left);
        }
        const Vec2 surfaceSize = Vec2((float)s_Window->GetWidth(), (float)s_Window->GetHeight());
        m_UIRenderer->Render(s_Window.Get(), m_UIRoot, surfaceSize, uiContext);
    }

    RenderingAPI::GetInstance()->Draw();

    s_Window->PollEvents();
    return !s_Window->ShouldClose();
}

void EditorEngine::Shutdown() {
    // Tear down the UI (releases its pipelines/buffers) before the surface and RHI go away.
    delete m_UIRoot;
    m_UIRoot = nullptr;
    delete m_UIRenderer;
    m_UIRenderer = nullptr;

    AssetManager::Get().Shutdown();
    s_Window = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}