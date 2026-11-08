#include "ViewportTab.h"
#include "EditorCamera.h"
#include "MajorTab.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIDropdown.h"
#include "UI/UIViewportSurface.h"
#include "UI/EditorDragDrop.h"
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
#include "GameFramework/Node3D.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "Assets/Asset.h"
#include "GameFramework/UICanvas.h"
#include "InputSystem/KeyboardDevice.h"
#include "ThemedWindow.h"
#include "Window.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderTargetTexture.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UIImage.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UIWorld.h"
#include "Core/Log.h"

#include <format>

static constexpr float s_ToolBarHeight = 30.0f;
static constexpr float s_ViewGroupWidth = 424.0f;
static constexpr float s_DefaultDropDistance = 8.0f;
static constexpr float s_MaxDropDistance = 200.0f;
static const Vec4 s_DesignClearColor = HexColor(0x1B1B1E);

VectorImage* ViewportTab::GetTabIcon() const {
    return EditorIcons::Viewport();
}

bool ViewportTab::IsPossessedByPlay() const {
    MajorTab* major = GetMajorTab();
    return major && major->GetPlayState() == PlayState::Playing;
}

CameraNode* ViewportTab::GetViewCamera() const {
    if (IsPossessedByPlay()) {
        World* world = GetEditedWorld();
        if (CameraNode* playCamera = world ? world->GetMainCamera() : nullptr) {
            return playCamera;
        }
    }
    return m_Camera.Get();
}

ViewportTab::ViewportTab() {
    m_Camera = Object::Create<EditorCamera>();
    m_Pipeline = RenderPipeline::Create();
    m_BufferViews = m_Pipeline->GetBufferViews();
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

    EditorDragDrop::AddDropZone(*m_ViewportArea,
        [this](Asset* InAsset) { return AcceptsDroppedAsset(InAsset); },
        [this](Asset* InAsset, const Vec2& InCursorPos) { SpawnDroppedAsset(InAsset, InCursorPos); });

    BuildRenderStats(*m_ViewportArea);
}

ViewportTab::~ViewportTab() {
    delete m_DesignRenderer;
    m_DesignRenderer = nullptr;
}

void ViewportTab::BuildSceneTools(UINode& InParent) {
    static const char* toolNames[] = { "Select", "Move", "Rotate", "Scale" };
    static const GizmoMode toolModes[] = { GizmoMode::Select, GizmoMode::Translate, GizmoMode::Rotate, GizmoMode::Scale };
    VectorImage* const toolIcons[] = { EditorIcons::Select(), EditorIcons::Move(),
                                       EditorIcons::Rotate(), EditorIcons::Scale() };
    for (int i = 0; i < 4; i++) {
        const GizmoMode mode = toolModes[i];
        UIButton& button = EditorStyle::ToolButton(InParent, toolIcons[i], toolNames[i],
                                                   [this, mode] { m_TransformGizmo->Mode = mode; });
        UIButton* buttonPtr = &button;
        button.Bind = [this, buttonPtr, mode] {
            EditorStyle::SetToggled(*buttonPtr, m_TransformGizmo->Mode == mode);
        };
    }

    UIButton& space = EditorStyle::ToolButton(InParent, EditorIcons::WorldSpace(), "World", [this] {
        m_TransformGizmo->Space = m_TransformGizmo->Space == GizmoSpace::World ? GizmoSpace::Local : GizmoSpace::World;
    });
    UISvg* spaceIcon = space.GetChildByClass(UISvg::StaticClass())->As<UISvg>();
    UITooltip* spaceTooltip = space.GetChildByClass(UITooltip::StaticClass())->As<UITooltip>();
    space.Bind = [this, spaceIcon, spaceTooltip] {
        const bool local = m_TransformGizmo->Space == GizmoSpace::Local || m_TransformGizmo->Mode == GizmoMode::Scale;
        spaceIcon->Image = local ? EditorIcons::LocalSpace() : EditorIcons::WorldSpace();
        spaceIcon->Tint = m_TransformGizmo->Mode == GizmoMode::Scale ? EditorStyle::TextDim : EditorStyle::Text;
        spaceTooltip->Text = local ? "Local" : "World";
    };
}

void ViewportTab::BuildDesignTools(UINode& InParent) {
    static const char* toolNames[] = { "Select", "Rect", "Rotate" };
    static const UILayoutTool tools[] = { UILayoutTool::Select, UILayoutTool::Rect, UILayoutTool::Rotate };
    VectorImage* const toolIcons[] = { EditorIcons::Select(), EditorIcons::RectTool(), EditorIcons::Rotate() };
    for (int i = 0; i < 3; i++) {
        const UILayoutTool tool = tools[i];
        UIButton& button = EditorStyle::ToolButton(InParent, toolIcons[i], toolNames[i],
                                                   [this, tool] { m_LayoutGizmo->Tool = tool; });
        UIButton* buttonPtr = &button;
        button.Bind = [this, buttonPtr, tool] {
            EditorStyle::SetToggled(*buttonPtr, m_LayoutGizmo->Tool == tool);
        };
    }

    UIButton& preview = EditorStyle::ToolButton(InParent, EditorIcons::Eye(), "Preview",
                                                [this] { m_PreviewInput = !m_PreviewInput; });
    UIButton* previewPtr = &preview;
    preview.Bind = [this, previewPtr] {
        EditorStyle::SetToggled(*previewPtr, m_PreviewInput);
    };
}

static String FormatTiming(const RenderTiming& InTiming) {
    return std::format("{:.2f} / {:.2f} ms", InTiming.CpuMilliseconds, InTiming.GpuMilliseconds);
}

void ViewportTab::BuildRenderStats(UINode& InParent) {
    constexpr float rowHeight = 18.0f;
    constexpr float toggleWidth = 30.0f;
    constexpr float timingWidth = 110.0f;

    RenderPipeline* pipeline = m_Pipeline.Get();
    int32_t rowCount = 1;
    for (RenderStage* stage = pipeline->GetFirstStage(); stage; stage = stage->GetNext()) {
        rowCount += 1 + stage->GetPasses().Size();
    }

    UIQuad* panel = InParent.Add<UIQuad>();
    panel->Anchor = panel->Pivot = Vec2(0.0f);
    panel->Position = Vec2(8.0f);
    panel->Size = { 340.0_px, UIValue(rowCount * rowHeight + 12.0f) };
    panel->Padding = UIPadding(8.0f, 6.0f);
    panel->Color = HexColor(0x161616, 0.85f);
    // BindTree skips disabled nodes, so the always-enabled parent has to do the toggling.
    InParent.Bind = [this, panel] { panel->SetEnabled(m_ShowRenderStats && !m_DesignMode); };

    UIVStack* rows = panel->Add<UIVStack>();
    rows->Fill();

    auto addRow = [&](const String& InName, float InIndent, std::function<const RenderTiming&()> InTiming, RenderPass* InPass) {
        UIHStack* row = rows->Add<UIHStack>();
        row->Size = { 1.0_rel, UIValue(rowHeight) };
        row->Padding = UIPadding(UIValue(InIndent), 0.0f, 0.0f, 0.0f);
        row->Gap = 4.0f;

        UIButton* toggle = InPass ? row->Add<UIButton>() : nullptr;
        UILabel* name = row->Add<UILabel>();
        UILabel* timing = row->Add<UILabel>();

        name->Text = InName;
        name->Size = { 1.0_rel - UIValue(timingWidth + (toggle ? toggleWidth + 8.0f : 4.0f)), 1.0_rel };
        timing->Size = { UIValue(timingWidth), 1.0_rel };
        timing->HAlign = UIHAlign::Right;
        timing->Bind = [timing, InTiming] { timing->Text = FormatTiming(InTiming()); };
        for (UILabel* label : { name, timing }) {
            label->FontSize = EditorStyle::FontSize;
            label->VAlign = UIVAlign::Middle;
            label->Color = EditorStyle::Text;
        }

        if (!toggle) {
            return;
        }
        toggle->Size = { UIValue(toggleWidth), 1.0_rel };
        toggle->SetCaption("On");
        EditorStyle::ApplyButtonStyle(*toggle, EditorStyle::FontSize - 2.0f);
        toggle->Clicked = [InPass] { InPass->SetEnabled(!InPass->IsEnabled()); };
        UILabel* caption = toggle->GetChildByClass(UILabel::StaticClass())->As<UILabel>();
        toggle->Bind = [InPass, toggle, caption, name] {
            EditorStyle::SetToggled(*toggle, InPass->IsEnabled());
            caption->Text = !InPass->IsSupported() ? "N/A" : InPass->IsEnabled() ? "On" : "Off";
            name->Color = InPass->IsEnabled() ? EditorStyle::Text : EditorStyle::TextDim;
        };
    };

    addRow("Frame  (CPU / GPU)", 0.0f, [pipeline]() -> const RenderTiming& { return pipeline->GetTiming(); }, nullptr);
    for (RenderStage* stage = pipeline->GetFirstStage(); stage; stage = stage->GetNext()) {
        addRow(stage->GetDisplayName(), 8.0f, [stage]() -> const RenderTiming& { return stage->GetTiming(); }, nullptr);
        for (const SharedObjectPtr<RenderPass>& pass : stage->GetPasses()) {
            RenderPass* raw = pass.Get();
            addRow(raw->GetName(), 16.0f, [raw]() -> const RenderTiming& { return raw->GetTiming(); }, raw);
        }
    }
}

static UIDropdownOption IconOption(const char* InLabel, VectorImage* InIcon) {
    UIDropdownOption option(InLabel);
    option.Icon = InIcon;
    option.IconTint = EditorStyle::Text;
    return option;
}

void ViewportTab::BuildToolBar(UINode& InToolBar) {
    UIHStack* sceneTools = InToolBar.Add<UIHStack>();
    sceneTools->Anchor = sceneTools->Pivot = Vec2(0.0f);
    sceneTools->Position = Vec2(0.0f);
    sceneTools->Size = { 1.0_rel - UIValue(s_ViewGroupWidth), 1.0_rel };
    sceneTools->Padding = UIPadding(4.0f, 3.0f);
    sceneTools->Gap = 2.0f;
    BuildSceneTools(*sceneTools);

    UIHStack* designTools = InToolBar.Add<UIHStack>();
    designTools->Anchor = designTools->Pivot = Vec2(0.0f);
    designTools->Position = Vec2(0.0f);
    designTools->Size = { 1.0_rel - UIValue(s_ViewGroupWidth), 1.0_rel };
    designTools->Padding = UIPadding(4.0f, 3.0f);
    designTools->Gap = 2.0f;
    BuildDesignTools(*designTools);

    UIHStack* viewGroup = InToolBar.Add<UIHStack>();
    viewGroup->Anchor = viewGroup->Pivot = Vec2(1.0f, 0.0f);
    viewGroup->Position = Vec2(0.0f);
    viewGroup->Size = { UIValue(s_ViewGroupWidth), 1.0_rel };
    viewGroup->Padding = UIPadding(4.0f, 3.0f);
    viewGroup->Gap = 6.0f;

    UIHStack* sceneView = viewGroup->Add<UIHStack>();
    sceneView->Size = { 338.0_px, 1.0_rel };
    sceneView->Gap = 4.0f;
    BuildSceneViewOptions(*sceneView);

    InToolBar.Bind = [this, sceneTools, designTools, sceneView] {
        sceneTools->SetEnabled(!m_DesignMode);
        designTools->SetEnabled(m_DesignMode);
        sceneView->SetEnabled(!m_DesignMode);
    };

    UIHStack* dimension = viewGroup->Add<UIHStack>();
    dimension->Size = { 64.0_px, 1.0_rel };
    for (const bool design : { false, true }) {
        UIButton& button = UI::Button(*dimension, design ? "UI" : "3D", [this, design] { m_DesignMode = design; });
        button.Size = { 32.0_px, 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
        EditorStyle::StyleSegment(button, !design, design);
        button.Add<UITooltip>()->Text = design ? "UI design (2)" : "Scene (2)";
        UIButton* buttonPtr = &button;
        button.Bind = [this, buttonPtr, design] {
            EditorStyle::SetToggled(*buttonPtr, m_DesignMode == design);
        };
    }
}

void ViewportTab::BuildSceneViewOptions(UINode& InParent) {
    UIDropdown* projection = InParent.Add<UIDropdown>();
    projection->Size = { 124.0_px, 1.0_rel };
    projection->GetOptions = [] {
        return Array<UIDropdownOption>{ IconOption("Perspective", EditorIcons::Perspective()),
                                        IconOption("Orthographic", EditorIcons::Orthographic()) };
    };
    projection->GetSelectedIndex = [this] { return m_Camera->IsOrthographicView() ? 1 : 0; };
    projection->GetSelectedLabel = [this] { return String(m_Camera->IsOrthographicView() ? "Orthographic" : "Perspective"); };
    projection->GetSelectedIcon = [this] {
        return m_Camera->IsOrthographicView() ? EditorIcons::Orthographic() : EditorIcons::Perspective();
    };
    projection->SelectionChanged = [this](int32_t InIndex) { m_Camera->SetOrthographicView(InIndex == 1); };
    projection->Add<UITooltip>()->Text = "Projection";

    UIDropdown* view = InParent.Add<UIDropdown>();
    view->Size = { 172.0_px, 1.0_rel };
    view->GetOptions = [this] {
        Array<UIDropdownOption> options = { IconOption("Lit", EditorIcons::Lit()) };
        for (const RenderBufferView& bufferView : m_BufferViews) {
            options.Add(IconOption(bufferView.Label.c_str(), EditorIcons::BufferView()));
        }
        return options;
    };
    view->GetSelectedIndex = [this] { return m_BufferViewIndex + 1; };
    view->GetSelectedLabel = [this] { return m_BufferViewIndex < 0 ? String("Lit") : m_BufferViews[m_BufferViewIndex].Label; };
    view->GetSelectedIcon = [this] { return m_BufferViewIndex < 0 ? EditorIcons::Lit() : EditorIcons::BufferView(); };
    view->SelectionChanged = [this](int32_t InIndex) { m_BufferViewIndex = InIndex - 1; };
    view->Add<UITooltip>()->Text = "View";

    UIButton& stats = EditorStyle::ToolButton(InParent, EditorIcons::Stats(), "Render stats",
                                              [this] { m_ShowRenderStats = !m_ShowRenderStats; });
    UIButton* statsPtr = &stats;
    stats.Bind = [this, statsPtr] { EditorStyle::SetToggled(*statsPtr, m_ShowRenderStats); };
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
    params.CameraOverride = GetViewCamera();
    params.BufferView = m_BufferViewIndex < 0 ? nullptr : &m_BufferViews[m_BufferViewIndex];

    Array<GizmoDraw> gizmos;
    if (!IsPossessedByPlay()) {
        m_GizmoLayer->Collect(GetEditedWorld(), m_Camera.Get(), GetMajorTab(), gizmos);
        m_Pipeline->Render(InContext.DeltaTime, params);
    } else {
        const Mat4 hostProjection = s_ViewProjection;
        const float hostWidth = s_ViewportW;
        const float hostHeight = s_ViewportH;
        RouteGameUI(InContext, Vec2((float)params.Width, (float)params.Height));
        m_Pipeline->Render(InContext.DeltaTime, params);
        SetViewProjection(hostProjection, hostWidth, hostHeight);

        m_TransformGizmo->EndDrag();
        m_TransformGizmo->ClearHover();
        UIWorld* ui = UIWorld::Get(GetEditedWorld());
        m_ViewportArea->Cursor = ui ? ui->GetDesiredCursor() : CursorIcon::Arrow;
        m_SceneTexture->SetView(m_Pipeline->GetFinalImageView());
        return;
    }

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

void ViewportTab::RouteGameUI(const UIFrameContext& InContext, const Vec2& InViewportSize) {
    UIWorld* ui = UIWorld::Get(GetEditedWorld());
    if (!ui) {
        return;
    }
    const bool hovered = m_ViewportArea->IsHovered();
    UIFrameContext context = m_ViewportArea->IsFocused() ? InContext : UIFrameContext();
    context.DeltaTime = InContext.DeltaTime;
    context.CursorPosition = m_ViewportArea->ToRenderPixel(InContext.CursorPosition);
    context.CursorDown = InContext.CursorDown;
    context.CursorPressedThisFrame = hovered && InContext.CursorPressedThisFrame;
    context.CursorReleasedThisFrame = InContext.CursorReleasedThisFrame;
    context.SecondaryDown = InContext.SecondaryDown;
    context.SecondaryPressedThisFrame = hovered && InContext.SecondaryPressedThisFrame;
    context.SecondaryReleasedThisFrame = InContext.SecondaryReleasedThisFrame;
    context.ScrollDelta = hovered ? InContext.ScrollDelta : Vec2(0.0f);
    ui->Route(InViewportSize, context);
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
    const bool possessed = IsPossessedByPlay();
    const bool focused = focusedWindow && focusedWindow->GetCanvas() == GetCanvas() && !possessed;
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
    if (IsPossessedByPlay()) {
        return;
    }
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
    if (IsPossessedByPlay()) {
        return;
    }
    if (m_DesignMode) {
        if (!m_PreviewInput) {
            RecordSelectionEdit("Layout");
            m_LayoutGizmo->Drag(CanvasFromViewport(InRenderPixel));
        }
        return;
    }
    RecordSelectionEdit("Transform");
    m_TransformGizmo->Drag(InRenderPixel);
}

void ViewportTab::RecordSelectionEdit(const String& InTitle) {
    if (MajorTab* major = GetMajorTab()) {
        for (Object* selected : major->GetSelection()) {
            RecordEdit(InTitle, selected);
        }
    }
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

Vec3 ViewportTab::DropPointAt(const Vec2& InRenderPixel) const {
    const Vec2 size = m_ViewportArea->GetGeometry().Size;
    if (size.x <= 0.0f || size.y <= 0.0f) {
        return Vec3(0.0f);
    }

    const Vec2 ndc = Vec2(InRenderPixel.x / size.x, InRenderPixel.y / size.y) * 2.0f - 1.0f;
    const Mat4 inverseViewProjection = glm::inverse(GetViewCamera()->GetViewProjectionMatrix());
    const Vec4 nearPoint = inverseViewProjection * Vec4(ndc.x, ndc.y, 0.0f, 1.0f);
    const Vec4 farPoint = inverseViewProjection * Vec4(ndc.x, ndc.y, 1.0f, 1.0f);
    const Vec3 origin = Vec3(nearPoint) / nearPoint.w;
    const Vec3 direction = glm::normalize(Vec3(farPoint) / farPoint.w - origin);

    const float toGround = (std::abs(direction.y) > 1e-4f) ? (-origin.y / direction.y) : -1.0f;
    const float distance = (toGround > 0.0f) ? std::min(toGround, s_MaxDropDistance) : s_DefaultDropDistance;
    return origin + direction * distance;
}

bool ViewportTab::AcceptsDroppedAsset(Asset* InAsset) const {
    MajorTab* major = GetMajorTab();
    return !m_DesignMode && major && major->GetAssetRootNode() && MajorTab::IsSpawnableAsset(InAsset);
}

void ViewportTab::SpawnDroppedAsset(Asset* InAsset, const Vec2& InCursorPos) {
    MajorTab* major = GetMajorTab();
    Node* root = major ? major->GetAssetRootNode() : nullptr;
    if (!root) {
        return;
    }

    RecordEdit("Add Node", root);
    Node* spawned = MajorTab::SpawnFromAsset(InAsset, *root);
    if (!spawned) {
        return;
    }
    if (Node3D* placed = Cast<Node3D>(spawned)) {
        placed->SetPosition(DropPointAt(m_ViewportArea->ToRenderPixel(InCursorPos)));
    }
    major->SetSelection(spawned);
}

bool ViewportTab::OnScroll(const Vec2& InDelta) {
    if (!m_ViewportArea->IsHovered()) {
        return false;
    }
    if (m_DesignMode || IsPossessedByPlay()) {
        return true;
    }
    if (!m_Camera->IsNavigating()) {
        m_Camera->Dolly(InDelta.y);
    }
    return true;
}
