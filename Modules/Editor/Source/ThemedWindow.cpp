#include "ThemedWindow.h"
#include "UI/EditorStyle.h"
#include "UI/UICaptionButton.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UIQuad.h"
#include "Rendering/UIRenderer.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/KeyCodes.h"

static Array<SharedObjectPtr<ThemedWindow>> s_WindowRegistry;

ThemedWindow::ThemedWindow(const WindowParams& InParams)
    : Window(InParams) {
    BuildChrome(InParams.Title);
}

ThemedWindow::~ThemedWindow() {
    ReleaseResources();
}

void ThemedWindow::ReleaseResources() {
    delete m_UIRenderer;
    m_UIRenderer = nullptr;
    delete m_Canvas;
    m_Canvas = nullptr;
}

SharedObjectPtr<ThemedWindow> ThemedWindow::Create(WindowParams InParams) {
    InParams.EditorStyle = true;
    SharedObjectPtr<ThemedWindow> window(new ThemedWindow(InParams));
    RegisterWindow(window);
    return window;
}

void ThemedWindow::BuildChrome(const String& InTitle) {
    m_Canvas = new UICanvas();

    UIVStack* root = m_Canvas->Add<UIVStack>();
    root->Fill();

    m_TitleBar = root->Add<UIQuad>();
    m_TitleBar->Size = { 1.0_rel, UIValue(EditorStyle::TitleBarHeight) };
    m_TitleBar->Color = EditorStyle::TitleBar;

    float labelRightInset = 12.0f;
#if !defined(AE_PLATFORM_MACOS)
    // Windows/Linux draw their own caption buttons; macOS keeps the native traffic lights,
    // which overlay the title bar thanks to the full-size content view.
    const UICaptionAction actions[] = { UICaptionAction::Minimize, UICaptionAction::Maximize, UICaptionAction::Close };
    for (int i = 0; i < 3; i++) {
        UICaptionButton* button = m_TitleBar->Add<UICaptionButton>();
        button->Action = actions[i];
        button->Anchor = Vec2(1.0f, 0.0f);
        button->Pivot = Vec2(1.0f, 0.0f);
        button->Position = Vec2(-(float)(2 - i) * EditorStyle::CaptionButtonWidth, 0.0f);
        button->Size = { UIValue(EditorStyle::CaptionButtonWidth), 1.0_rel };
        switch (button->Action) {
            case UICaptionAction::Minimize: button->Clicked = [this] { Minimize(); }; break;
            case UICaptionAction::Maximize: button->Clicked = [this] { ToggleMaximize(); }; break;
            case UICaptionAction::Close: button->Clicked = [this] { Close(); }; break;
        }
        m_TitleBarButtons.Add(button);
    }
    labelRightInset += 3.0f * EditorStyle::CaptionButtonWidth;
#endif

    m_TitleLabel = m_TitleBar->Add<UILabel>();
    m_TitleLabel->Text = InTitle;
    m_TitleLabel->FontSize = EditorStyle::FontSize;
    m_TitleLabel->Color = EditorStyle::TextDim;
    m_TitleLabel->HAlign = UIHAlign::Right;
    m_TitleLabel->VAlign = UIVAlign::Middle;
    m_TitleLabel->Anchor = Vec2(1.0f, 0.0f);
    m_TitleLabel->Pivot = Vec2(1.0f, 0.0f);
    m_TitleLabel->Position = Vec2(-labelRightInset, 0.0f);
    m_TitleLabel->Size = { 300.0_px, 1.0_rel };

    m_ContentRoot = root->Add<UINode>();
    m_ContentRoot->Size = { 1.0_rel, 1.0_rel };
}

void ThemedWindow::SetTitleText(const String& InText) {
    m_TitleLabel->Text = InText;
}

void ThemedWindow::BuildFrameContext(UIFrameContext& OutContext, double InDeltaTime) {
    OutContext.DeltaTime = (float)InDeltaTime;
    OutContext.CursorPosition = GetCursorPosition();
    OutContext.CursorDown = IsMouseButtonDown(0);
    OutContext.CursorPressedThisFrame = OutContext.CursorDown && !m_WasCursorDown;
    OutContext.CursorReleasedThisFrame = !OutContext.CursorDown && m_WasCursorDown;
    OutContext.ScrollDelta = ConsumeScrollDelta();
    OutContext.TextInput = ConsumeTextInput();
    m_WasCursorDown = OutContext.CursorDown;

    if (IsFocused()) {
        if (KeyboardDevice* keyboard = KeyboardDevice::Instance()) {
            OutContext.NavUp = keyboard->IsDown(KeyCode::Up);
            OutContext.NavDown = keyboard->IsDown(KeyCode::Down);
            OutContext.NavLeft = keyboard->IsDown(KeyCode::Left);
            OutContext.NavRight = keyboard->IsDown(KeyCode::Right);
            OutContext.NavSelectPressed = keyboard->IsDown(KeyCode::Enter);
            OutContext.NavSelectReleased = keyboard->IsUp(KeyCode::Enter);
            OutContext.NavBack = keyboard->IsDown(KeyCode::Escape);
        }
    }
}

void ThemedWindow::RenderWindow(double InDeltaTime) {
    if (!IsVisible() || IsMinimized() || GetWidth() == 0 || GetHeight() == 0) {
        return;
    }
    m_FrameSeconds = InDeltaTime;
    if (!m_UIRenderer) {
        m_UIRenderer = new UIRenderer();
    }

    PreUIRender(InDeltaTime);

    UIFrameContext context;
    BuildFrameContext(context, InDeltaTime);
    const Vec2 surfaceSize = Vec2((float)GetWidth(), (float)GetHeight());
    m_UIRenderer->Render(this, m_Canvas, surfaceSize, context);
    SetCursorIcon(m_Canvas->GetDesiredCursor());
}

bool ThemedWindow::HitTestTitleBar(const Vec2& InPoint) const {
    if (!m_TitleBar || !m_TitleBar->GetGeometry().Contains(InPoint)) {
        return false;
    }
#if defined(AE_PLATFORM_MACOS)
    // The OS owns the traffic-light buttons in the top-left; claiming that as a drag region makes
    // our window-drag intercept swallow their clicks (worst on non-focused windows, which accept
    // first-mouse), so the buttons stop responding.
    if (InPoint.x < EditorStyle::MacTrafficLightInset) {
        return false;
    }
#endif
    for (UINode* button : m_TitleBarButtons) {
        if (button->GetGeometry().Contains(InPoint)) {
            return false;
        }
    }
    return true;
}

bool ThemedWindow::ContainsScreenPoint(const Vec2& InScreenPoint) const {
    const Vec2 local = InScreenPoint - GetPosition();
    return local.x >= 0.0f && local.y >= 0.0f && local.x < (float)GetWidth() && local.y < (float)GetHeight();
}

ThemedWindow* ThemedWindow::FindWindowAtScreenPoint(const Vec2& InScreenPoint, const ThemedWindow* InIgnored) {
    // Prefer the focused window: GLFW has no z-order query, and the focused one is on top.
    ThemedWindow* hit = nullptr;
    for (const SharedObjectPtr<ThemedWindow>& window : s_WindowRegistry) {
        ThemedWindow* candidate = window.Get();
        if (!candidate || candidate == InIgnored || !candidate->IsVisible()) {
            continue;
        }
        if (!candidate->ContainsScreenPoint(InScreenPoint)) {
            continue;
        }
        if (candidate->IsFocused()) {
            return candidate;
        }
        if (!hit) {
            hit = candidate;
        }
    }
    return hit;
}

const Array<SharedObjectPtr<ThemedWindow>>& ThemedWindow::GetAllWindows() {
    return s_WindowRegistry;
}

void ThemedWindow::RegisterWindow(const SharedObjectPtr<ThemedWindow>& InWindow) {
    s_WindowRegistry.Add(InWindow);
}

void ThemedWindow::DestroyWindow(ThemedWindow* InWindow) {
    // Hold a strong ref across the registry removal so the actual delete happens after the
    // vector is fully consistent.
    for (int32_t i = 0; i < s_WindowRegistry.Size(); i++) {
        if (s_WindowRegistry[i].Get() == InWindow) {
            SharedObjectPtr<ThemedWindow> keepAlive = s_WindowRegistry[i];
            s_WindowRegistry.RemoveAt(i);
            keepAlive->ReleaseResources();
            return;
        }
    }
}

void ThemedWindow::DestroyAllWindows() {
    while (!s_WindowRegistry.IsEmpty()) {
        DestroyWindow(s_WindowRegistry[s_WindowRegistry.Size() - 1].Get());
    }
}
