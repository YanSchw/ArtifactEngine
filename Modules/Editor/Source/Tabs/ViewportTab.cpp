#include "ViewportTab.h"
#include "EditorCamera.h"
#include "MajorTab.h"
#include "UI/EditorStyle.h"
#include "UI/UIViewportSurface.h"
#include "Gizmos/GizmoLayer.h"
#include "Gizmos/GizmoRenderer.h"
#include "Rendering/FrameBuffer.h"
#include "GameFramework/Node.h"
#include "InputSystem/KeyboardDevice.h"
#include "ThemedWindow.h"
#include "Window.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderTargetTexture.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UIImage.h"
#include "GameFramework/UIBuilder.h"
#include "Core/Log.h"

static constexpr float s_ToolBarHeight = 30.0f;

ViewportTab::ViewportTab() {
    m_Camera = Object::Create<EditorCamera>();
    m_Pipeline = Object::Create(EngineConfig::RenderPipelineClass())->As<RenderPipeline>();
    AE_ASSERT(m_Pipeline, "Failed to create RenderPipeline instance!");
    m_SceneTexture = Object::Create<RenderTargetTexture>();
    m_GizmoLayer = Object::Create<GizmoLayer>();
    m_GizmoRenderer = Object::Create<GizmoRenderer>();

    UIVStack* layout = Add<UIVStack>();
    layout->Fill();

    UIQuad* toolBar = layout->Add<UIQuad>();
    toolBar->Size = { 1.0_rel, s_ToolBarHeight };
    toolBar->Color = EditorStyle::ToolBar;
    BuildToolBar(*toolBar);

    m_ViewportArea = layout->Add<UIViewportSurface>();
    m_ViewportArea->Size = { 1.0_rel, 1.0_rel };  // whatever the toolbar leaves over
    m_ViewportArea->Image = m_SceneTexture;
    m_ViewportArea->Cursor = CursorIcon::Crosshair;
    m_ViewportArea->Pressed = [this](const Vec2& InRenderPixel) { PickAt(InRenderPixel); };
}

void ViewportTab::BuildToolBar(UINode& InToolBar) {
    UIHStack* tools = InToolBar.Add<UIHStack>();
    tools->Anchor = tools->Pivot = Vec2(0.0f);
    tools->Position = Vec2(0.0f);
    tools->Size = { 1.0_rel - 160.0_px, 1.0_rel };
    tools->Padding = UIPadding(4.0f, 3.0f);
    tools->Gap = 2.0f;

    static const char* toolNames[] = { "Select", "Move", "Rotate", "Scale" };
    for (int i = 0; i < 4; i++) {
        UIButton& button = UI::Button(*tools, toolNames[i], [this, i] { m_ActiveTool = i; });
        button.Size = { 58.0_px, 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
        UIButton* buttonPtr = &button;
        button.Bind = [this, buttonPtr, i] {
            buttonPtr->NormalColor = (m_ActiveTool == i) ? EditorStyle::Accent : EditorStyle::Button;
        };
    }

    UIHStack* viewGroup = InToolBar.Add<UIHStack>();
    viewGroup->Anchor = viewGroup->Pivot = Vec2(1.0f, 0.0f);
    viewGroup->Position = Vec2(0.0f);
    viewGroup->Size = { 160.0_px, 1.0_rel };
    viewGroup->Padding = UIPadding(4.0f, 3.0f);
    viewGroup->Gap = 2.0f;

    UIButton& perspective = UI::Button(*viewGroup, "Perspective", [] { AE_INFO("ViewportTab: Perspective"); });
    perspective.Size = { 96.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(perspective);

    UIButton& lit = UI::Button(*viewGroup, "Lit", [] { AE_INFO("ViewportTab: Lit"); });
    lit.Size = { 54.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(lit);
}

void ViewportTab::OnUIUpdate(const UIFrameContext& InContext) {
    // Keyboard and right/middle mouse are polled globally, so only the focused window's
    // viewport may drive the camera.
    ThemedWindow* focusedWindow = Cast<ThemedWindow>(Window::GetFocusedWindow());
    if (focusedWindow && focusedWindow->GetCanvas() == GetCanvas()) {
        m_Camera->UpdateNavigation(InContext.DeltaTime, *focusedWindow, m_ViewportArea->IsHovered());
    } else if (m_Camera->IsNavigating()) {
        m_Camera->CancelNavigation();
    }

    // Render the scene at the viewport's size; the scene commands enter the queue here,
    // ahead of this window's UI pass that samples the result.
    const UIRectF& rect = m_ViewportArea->GetGeometry();
    RenderParams params;
    params.Width = (uint32_t)glm::max(rect.Size.x, 1.0f);
    params.Height = (uint32_t)glm::max(rect.Size.y, 1.0f);
    params.m_World = GetEditedWorld();
    params.CameraOverride = m_Camera.Get();
    m_Pipeline->Render(InContext.DeltaTime, params);

    Array<GizmoDraw> gizmos;
    m_GizmoLayer->Collect(GetEditedWorld(), m_Camera.Get(), GetMajorTab(), gizmos);
    m_GizmoRenderer->Render(m_Pipeline->GetFrameBuffer().Get(), m_Camera.Get(), gizmos);

    m_SceneTexture->SetView(m_Pipeline->GetFinalImageView());
}

void ViewportTab::PickAt(const Vec2& InRenderPixel) {
    MajorTab* major = GetMajorTab();
    if (!major || !m_Pipeline.Get() || InRenderPixel.x < 0.0f || InRenderPixel.y < 0.0f) {
        return;
    }

    Node* picked = Node::FindById(m_Pipeline->PickNodeId((uint32_t)InRenderPixel.x, (uint32_t)InRenderPixel.y));

    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    const bool toggle = keyboard && (keyboard->IsPressed(KeyCode::LeftControl) || keyboard->IsPressed(KeyCode::RightControl)
                                  || keyboard->IsPressed(KeyCode::LeftSuper) || keyboard->IsPressed(KeyCode::RightSuper));

    if (!picked) {
        if (!toggle) {
            major->ClearSelection();
        }
        return;
    }
    if (toggle) {
        major->ToggleSelection(picked);
    } else {
        major->SetSelection(picked);
    }
}

bool ViewportTab::OnScroll(const Vec2& InDelta) {
    if (!m_ViewportArea->IsHovered()) {
        return false;
    }
    if (!m_Camera->IsNavigating()) {
        m_Camera->Dolly(InDelta.y);
    }
    return true;
}
