#include "ViewportTab.h"
#include "EditorCamera.h"
#include "MajorTab.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIViewportSurface.h"
#include "Gizmos/GizmoLayer.h"
#include "Gizmos/GizmoRenderer.h"
#include "Gizmos/TransformGizmo.h"
#include "Gizmos/UILayoutGizmo.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Image.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/UIDrawList.h"
#include "Rendering/UIRenderer.h"
#include "GameFramework/Node.h"
#include "GameFramework/World.h"
#include "GameFramework/UICanvas.h"
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
static const Vec4 s_DesignClearColor = HexColor(0x1B1B1E);

VectorImage* ViewportTab::GetTabIcon() const {
    return EditorIcons::Viewport();
}

ViewportTab::ViewportTab() {
    m_Camera = Object::Create<EditorCamera>();
    m_Pipeline = Object::Create(EngineConfig::RenderPipelineClass())->As<RenderPipeline>();
    AE_ASSERT(m_Pipeline, "Failed to create RenderPipeline instance!");
    m_SceneTexture = Object::Create<RenderTargetTexture>();
    m_GizmoLayer = Object::Create<GizmoLayer>();
    m_GizmoRenderer = Object::Create<GizmoRenderer>();
    m_TransformGizmo = Object::Create<TransformGizmo>();
    m_LayoutGizmo = Object::Create<UILayoutGizmo>();
    m_DesignCanvas = Object::Create<UICanvas>();

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
    m_ViewportArea->Pressed = [this](const Vec2& InRenderPixel) { OnViewportPressed(InRenderPixel); };
    m_ViewportArea->Dragged = [this](const Vec2& InRenderPixel, const Vec2&) { OnViewportDragged(InRenderPixel); };
    m_ViewportArea->Released = [this](bool) { OnViewportReleased(); };
}

ViewportTab::~ViewportTab() {
    delete m_DesignRenderer;
    m_DesignRenderer = nullptr;
}

void ViewportTab::BuildSceneTools(UINode& InParent) {
    static const char* toolNames[] = { "Select", "Move", "Rotate", "Scale" };
    static const GizmoMode toolModes[] = { GizmoMode::Select, GizmoMode::Translate, GizmoMode::Rotate, GizmoMode::Scale };
    for (int i = 0; i < 4; i++) {
        const GizmoMode mode = toolModes[i];
        UIButton& button = UI::Button(InParent, toolNames[i], [this, mode] { m_TransformGizmo->Mode = mode; });
        button.Size = { 58.0_px, 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
        UIButton* buttonPtr = &button;
        button.Bind = [this, buttonPtr, mode] {
            buttonPtr->NormalColor = (m_TransformGizmo->Mode == mode) ? EditorStyle::Accent : EditorStyle::Button;
        };
    }

    UIButton& space = UI::Button(InParent, "World", [this] {
        m_TransformGizmo->Space = m_TransformGizmo->Space == GizmoSpace::World ? GizmoSpace::Local : GizmoSpace::World;
    });
    space.Size = { 58.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(space);
    if (Node* caption = space.GetChildByClass(UILabel::StaticClass())) {
        UILabel* spaceLabel = caption->As<UILabel>();
        space.Bind = [this, spaceLabel] {
            const bool local = m_TransformGizmo->Space == GizmoSpace::Local || m_TransformGizmo->Mode == GizmoMode::Scale;
            spaceLabel->Text = local ? "Local" : "World";
            spaceLabel->Color = m_TransformGizmo->Mode == GizmoMode::Scale ? EditorStyle::TextDim : EditorStyle::Text;
        };
    }
}

void ViewportTab::BuildDesignTools(UINode& InParent) {
    static const char* toolNames[] = { "Select", "Rect", "Rotate" };
    static const UILayoutTool tools[] = { UILayoutTool::Select, UILayoutTool::Rect, UILayoutTool::Rotate };
    for (int i = 0; i < 3; i++) {
        const UILayoutTool tool = tools[i];
        UIButton& button = UI::Button(InParent, toolNames[i], [this, tool] { m_LayoutGizmo->Tool = tool; });
        button.Size = { 58.0_px, 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
        UIButton* buttonPtr = &button;
        button.Bind = [this, buttonPtr, tool] {
            buttonPtr->NormalColor = (m_LayoutGizmo->Tool == tool) ? EditorStyle::Accent : EditorStyle::Button;
        };
    }

    UIButton& preview = UI::Button(InParent, "Preview", [this] { m_PreviewInput = !m_PreviewInput; });
    preview.Size = { 64.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(preview);
    UIButton* previewPtr = &preview;
    preview.Bind = [this, previewPtr] {
        previewPtr->NormalColor = m_PreviewInput ? EditorStyle::Accent : EditorStyle::Button;
    };
}

void ViewportTab::BuildToolBar(UINode& InToolBar) {
    UIHStack* sceneTools = InToolBar.Add<UIHStack>();
    sceneTools->Anchor = sceneTools->Pivot = Vec2(0.0f);
    sceneTools->Position = Vec2(0.0f);
    sceneTools->Size = { 1.0_rel - 220.0_px, 1.0_rel };
    sceneTools->Padding = UIPadding(4.0f, 3.0f);
    sceneTools->Gap = 2.0f;
    BuildSceneTools(*sceneTools);

    UIHStack* designTools = InToolBar.Add<UIHStack>();
    designTools->Anchor = designTools->Pivot = Vec2(0.0f);
    designTools->Position = Vec2(0.0f);
    designTools->Size = { 1.0_rel - 220.0_px, 1.0_rel };
    designTools->Padding = UIPadding(4.0f, 3.0f);
    designTools->Gap = 2.0f;
    BuildDesignTools(*designTools);

    InToolBar.Bind = [this, sceneTools, designTools] {
        sceneTools->SetEnabled(!m_DesignMode);
        designTools->SetEnabled(m_DesignMode);
    };

    UIHStack* viewGroup = InToolBar.Add<UIHStack>();
    viewGroup->Anchor = viewGroup->Pivot = Vec2(1.0f, 0.0f);
    viewGroup->Position = Vec2(0.0f);
    viewGroup->Size = { 220.0_px, 1.0_rel };
    viewGroup->Padding = UIPadding(4.0f, 3.0f);
    viewGroup->Gap = 2.0f;

    UIButton& dimension = UI::Button(*viewGroup, "3D", [this] { m_DesignMode = !m_DesignMode; });
    dimension.Size = { 44.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(dimension);
    UIButton* dimensionPtr = &dimension;
    if (Node* caption = dimension.GetChildByClass(UILabel::StaticClass())) {
        UILabel* label = caption->As<UILabel>();
        dimension.Bind = [this, dimensionPtr, label] {
            label->Text = m_DesignMode ? "2D" : "3D";
            dimensionPtr->NormalColor = m_DesignMode ? EditorStyle::Accent : EditorStyle::Button;
        };
    }

    UIButton& perspective = UI::Button(*viewGroup, "Perspective", [] { AE_INFO("ViewportTab: Perspective"); });
    perspective.Size = { 96.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(perspective);

    UIButton& lit = UI::Button(*viewGroup, "Lit", [] { AE_INFO("ViewportTab: Lit"); });
    lit.Size = { 54.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(lit);
}

UICanvas* ViewportTab::FindWorldCanvas() const {
    if (World* world = GetEditedWorld()) {
        for (Node* node : world->GetAllNodes()) {
            if (UICanvas* canvas = node->As<UICanvas>()) {
                return canvas;
            }
        }
    }
    return nullptr;
}

void ViewportTab::CollectDesignRoots(Array<UINode*>& OutRoots) const {
    World* world = GetEditedWorld();
    if (!world) {
        return;
    }
    for (Node* node : world->GetAllNodes()) {
        UINode* uiNode = node->As<UINode>();
        if (!uiNode || uiNode->As<UICanvas>()) {
            continue;
        }
        if (!node->GetParent() || !node->GetParent()->As<UINode>()) {
            OutRoots.Add(uiNode);
        }
    }
}

void ViewportTab::EnsureDesignTarget(uint32_t InWidth, uint32_t InHeight) {
    FrameBuffer* existing = m_DesignTarget.Get();
    if (existing && existing->GetDesc().Width == InWidth && existing->GetDesc().Height == InHeight) {
        return;
    }
    if (existing) {
        RenderingAPI::GetInstance()->WaitIdle();
    }

    ImageDesc imageDesc;
    imageDesc.Width = InWidth;
    imageDesc.Height = InHeight;
    imageDesc.Format = ImageFormat::RGBA8;
    imageDesc.Usage = ImageUsage::ColorAttachment | ImageUsage::Sampled;

    ImageViewDesc viewDesc;
    viewDesc.ImagePtr = Image::Create(imageDesc);
    viewDesc.Format = ImageFormat::RGBA8;

    FrameBufferDesc desc;
    desc.Width = InWidth;
    desc.Height = InHeight;
    desc.ColorAttachments.Add(ImageView::Create(viewDesc));
    desc.ClearColor = s_DesignClearColor;
    m_DesignTarget = FrameBuffer::Create(desc);
}

Vec2 ViewportTab::DesignCanvasSize() const {
    return Vec2(glm::abs(m_DesignProjection[0][0]) > 1e-6f ? 2.0f / m_DesignProjection[0][0] : m_DesignViewport.x,
                glm::abs(m_DesignProjection[1][1]) > 1e-6f ? 2.0f / m_DesignProjection[1][1] : m_DesignViewport.y);
}

Vec2 ViewportTab::CanvasFromViewport(const Vec2& InViewportPixel) const {
    const Vec2 canvasSize = DesignCanvasSize();
    return Vec2(InViewportPixel.x * canvasSize.x / glm::max(m_DesignViewport.x, 1.0f),
                InViewportPixel.y * canvasSize.y / glm::max(m_DesignViewport.y, 1.0f));
}

float ViewportTab::CanvasPixelScale() const {
    return glm::max(DesignCanvasSize().x / glm::max(m_DesignViewport.x, 1.0f), 1e-4f);
}

void ViewportTab::RenderScene(const UIFrameContext& InContext, const UIRectF& InRect) {
    RenderParams params;
    params.Width = (uint32_t)glm::max(InRect.Size.x, 1.0f);
    params.Height = (uint32_t)glm::max(InRect.Size.y, 1.0f);
    params.m_World = GetEditedWorld();
    params.CameraOverride = m_Camera.Get();
    m_Pipeline->Render(InContext.DeltaTime, params);

    Array<GizmoDraw> gizmos;
    m_GizmoLayer->Collect(GetEditedWorld(), m_Camera.Get(), GetMajorTab(), gizmos);
    m_GizmoRenderer->Render(m_Pipeline->GetFrameBuffer().Get(), m_Camera.Get(), gizmos);

    m_TransformGizmo->Update(GetMajorTab(), m_Camera.Get(), InRect.Size);
    if (m_ViewportArea->IsHovered() && !m_Camera->IsNavigating()) {
        m_TransformGizmo->SetHover(m_ViewportArea->ToRenderPixel(InContext.CursorPosition));
    } else if (!m_TransformGizmo->IsDragging()) {
        m_TransformGizmo->ClearHover();
    }
    m_ViewportArea->Cursor = m_TransformGizmo->IsEngaged() ? CursorIcon::Hand : CursorIcon::Crosshair;
    m_GizmoRenderer->RenderOverlay(m_Pipeline->GetFrameBuffer().Get(), m_Camera.Get(), m_TransformGizmo->BuildGeometry());

    m_SceneTexture->SetView(m_Pipeline->GetFinalImageView());
}

void ViewportTab::RenderDesign(const UIFrameContext& InContext, const UIRectF& InRect) {
    UICanvas* worldCanvas = FindWorldCanvas();
    UICanvas* canvas = worldCanvas ? worldCanvas : m_DesignCanvas.Get();
    if (!canvas) {
        return;
    }
    const uint32_t width = (uint32_t)glm::max(InRect.Size.x, 1.0f);
    const uint32_t height = (uint32_t)glm::max(InRect.Size.y, 1.0f);
    EnsureDesignTarget(width, height);
    if (!m_DesignRenderer) {
        m_DesignRenderer = new UIRenderer();
    }

    UIFrameContext designContext;
    designContext.DeltaTime = InContext.DeltaTime;
    designContext.CursorPosition = Vec2(-1.0e6f);
    if (m_PreviewInput) {
        designContext = InContext;
        designContext.CursorPosition = m_ViewportArea->ToRenderPixel(InContext.CursorPosition);
    }

    const Mat4 hostProjection = s_ViewProjection;
    const float hostWidth = s_ViewportW;
    const float hostHeight = s_ViewportH;

    m_DesignViewport = Vec2((float)width, (float)height);
    UIDrawList drawList;
    m_DesignProjection = canvas->RunFrame(m_DesignViewport, designContext, drawList);

    const UIRectF canvasRect = UIRectF(Vec2(0.0f), DesignCanvasSize());
    Array<UINode*> roots;
    CollectDesignRoots(roots);
    for (UINode* root : roots) {
        root->RunSubtreeFrame(canvasRect, designContext, drawList);
    }
    if (worldCanvas) {
        roots.Add(worldCanvas);
    }

    m_LayoutGizmo->Update(GetMajorTab(), roots, canvasRect, CanvasPixelScale());
    if (!m_PreviewInput) {
        if (m_ViewportArea->IsHovered()) {
            m_LayoutGizmo->SetHover(CanvasFromViewport(m_ViewportArea->ToRenderPixel(InContext.CursorPosition)));
        } else {
            m_LayoutGizmo->ClearHover();
        }
        m_LayoutGizmo->Paint(drawList);
    }

    SetViewProjection(hostProjection, hostWidth, hostHeight);

    m_ViewportArea->Cursor = m_PreviewInput ? canvas->GetDesiredCursor() : m_LayoutGizmo->GetCursor();
    m_DesignRenderer->Submit(m_DesignTarget.Get(), m_DesignViewport, drawList, m_DesignProjection);
    m_SceneTexture->SetView(m_DesignTarget->GetDesc().ColorAttachments[0]);
}

void ViewportTab::OnUIUpdate(const UIFrameContext& InContext) {
    // Keyboard and right/middle mouse are polled globally, so only the focused window's
    // viewport may drive the camera.
    ThemedWindow* focusedWindow = Cast<ThemedWindow>(Window::GetFocusedWindow());
    const bool focused = focusedWindow && focusedWindow->GetCanvas() == GetCanvas();
    if (focused && !m_DesignMode) {
        m_Camera->UpdateNavigation(InContext.DeltaTime, *focusedWindow, m_ViewportArea->IsHovered());
    } else if (m_Camera->IsNavigating()) {
        m_Camera->CancelNavigation();
    }
    if (focused && m_ViewportArea->IsHovered() && !m_Camera->IsNavigating() && !m_PreviewInput) {
        UpdateToolShortcuts();
    }

    const UIRectF& rect = m_ViewportArea->GetGeometry();
    if (m_DesignMode) {
        RenderDesign(InContext, rect);
    } else {
        RenderScene(InContext, rect);
    }
}

void ViewportTab::UpdateToolShortcuts() {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (!keyboard || m_TransformGizmo->IsDragging() || m_LayoutGizmo->IsDragging()) {
        return;
    }
    if (keyboard->IsDown(KeyCode::D2)) {
        m_DesignMode = !m_DesignMode;
        return;
    }
    if (m_DesignMode) {
        if (keyboard->IsDown(KeyCode::Q)) {
            m_LayoutGizmo->Tool = UILayoutTool::Select;
        } else if (keyboard->IsDown(KeyCode::T) || keyboard->IsDown(KeyCode::W)) {
            m_LayoutGizmo->Tool = UILayoutTool::Rect;
        } else if (keyboard->IsDown(KeyCode::E)) {
            m_LayoutGizmo->Tool = UILayoutTool::Rotate;
        }
        return;
    }
    if (keyboard->IsDown(KeyCode::Q)) {
        m_TransformGizmo->Mode = GizmoMode::Select;
    } else if (keyboard->IsDown(KeyCode::W)) {
        m_TransformGizmo->Mode = GizmoMode::Translate;
    } else if (keyboard->IsDown(KeyCode::E)) {
        m_TransformGizmo->Mode = GizmoMode::Rotate;
    } else if (keyboard->IsDown(KeyCode::R)) {
        m_TransformGizmo->Mode = GizmoMode::Scale;
    } else if (keyboard->IsDown(KeyCode::X)) {
        m_TransformGizmo->Space = m_TransformGizmo->Space == GizmoSpace::World ? GizmoSpace::Local : GizmoSpace::World;
    }
}

static bool IsToggleModifierHeld() {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && (keyboard->IsPressed(KeyCode::LeftControl) || keyboard->IsPressed(KeyCode::RightControl)
                     || keyboard->IsPressed(KeyCode::LeftSuper) || keyboard->IsPressed(KeyCode::RightSuper));
}

void ViewportTab::OnViewportPressed(const Vec2& InRenderPixel) {
    if (m_DesignMode) {
        if (m_PreviewInput) {
            return;
        }
        const Vec2 canvasPoint = CanvasFromViewport(InRenderPixel);
        if (!m_LayoutGizmo->BeginDrag(canvasPoint)) {
            PickUIAt(canvasPoint);
        }
        return;
    }
    if (!m_TransformGizmo->BeginDrag(InRenderPixel)) {
        PickAt(InRenderPixel);
    }
}

void ViewportTab::OnViewportDragged(const Vec2& InRenderPixel) {
    if (m_DesignMode) {
        if (!m_PreviewInput) {
            m_LayoutGizmo->Drag(CanvasFromViewport(InRenderPixel));
        }
        return;
    }
    m_TransformGizmo->Drag(InRenderPixel);
}

void ViewportTab::OnViewportReleased() {
    m_LayoutGizmo->EndDrag();
    m_TransformGizmo->EndDrag();
}

void ViewportTab::PickUIAt(const Vec2& InCanvasPoint) {
    MajorTab* major = GetMajorTab();
    if (!major) {
        return;
    }
    Node* picked = m_LayoutGizmo->Pick(InCanvasPoint);
    const bool toggle = IsToggleModifierHeld();

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

void ViewportTab::PickAt(const Vec2& InRenderPixel) {
    MajorTab* major = GetMajorTab();
    if (!major || !m_Pipeline.Get() || InRenderPixel.x < 0.0f || InRenderPixel.y < 0.0f) {
        return;
    }

    Node* picked = Node::FindById(m_Pipeline->PickNodeId((uint32_t)InRenderPixel.x, (uint32_t)InRenderPixel.y));
    const bool toggle = IsToggleModifierHeld();

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
    if (m_DesignMode) {
        return true;
    }
    if (!m_Camera->IsNavigating()) {
        m_Camera->Dolly(InDelta.y);
    }
    return true;
}
