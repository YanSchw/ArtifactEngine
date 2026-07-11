#include "EditorWindow.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorStyle.h"
#include "UI/UICaptionButton.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UIQuad.h"
#include "Assets/Font.h"
#include <cmath>
#include <string>

static void ClearChildren(UINode* InNode) {
    while (InNode->HasChildren()) {
        delete InNode->GetChild(0);
    }
}

EditorWindow::EditorWindow(const WindowParams& InParams)
    : Window(InParams) {
    BuildChrome();
}

EditorWindow::~EditorWindow() {
    delete m_Canvas;
    m_Canvas = nullptr;
}

SharedObjectPtr<EditorWindow> EditorWindow::Create(WindowParams InParams) {
    InParams.EditorStyle = true;
    return SharedObjectPtr<EditorWindow>(new EditorWindow(InParams));
}

void EditorWindow::BuildChrome() {
    m_Canvas = new UICanvas();

    UIVStack* root = m_Canvas->Add<UIVStack>();
    root->Fill();

    // Title bar: draggable chrome; the label sits top right, before the caption buttons.
    m_TitleBar = root->Add<UIQuad>();
    m_TitleBar->Size = { 1.0_rel, UIValue(EditorStyle::TitleBarHeight) };
    m_TitleBar->Color = EditorStyle::TitleBar;

    float labelRightInset = 12.0f;
#if !defined(AE_PLATFORM_MACOS)
    // Windows/Linux draw their own caption buttons; macOS keeps the native traffic lights
    // (upper left), which overlay the title bar thanks to the full-size content view.
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

    UILabel* title = m_TitleBar->Add<UILabel>();
    title->Text = "Artifact Editor";
    title->FontSize = EditorStyle::FontSize;
    title->Color = EditorStyle::TextDim;
    title->HAlign = UIHAlign::Right;
    title->VAlign = UIVAlign::Middle;
    title->Anchor = Vec2(1.0f, 0.0f);
    title->Pivot = Vec2(1.0f, 0.0f);
    title->Position = Vec2(-labelRightInset, 0.0f);
    title->Size = { 300.0_px, 1.0_rel };

    // Tool bar: content is rebuilt by the active MajorTab.
    UIQuad* toolBar = root->Add<UIQuad>();
    toolBar->Size = { 1.0_rel, UIValue(EditorStyle::ToolBarHeight) };
    toolBar->Color = EditorStyle::ToolBar;
    m_ToolBarContent = toolBar->Add<UIHStack>();
    m_ToolBarContent->Fill();
    m_ToolBarContent->Padding = UIPadding(6.0f, 6.0f);
    m_ToolBarContent->Gap = 6.0f;

    // Workspace: the active MajorTab fills it; paints nothing itself.
    m_TabHost = root->Add<UINode>();
    m_TabHost->Size = { 1.0_rel, 1.0_rel };

    // Bottom bar: every open MajorTab, plus frame stats on the right.
    UIQuad* bottomBar = root->Add<UIQuad>();
    bottomBar->Size = { 1.0_rel, UIValue(EditorStyle::BottomBarHeight) };
    bottomBar->Color = EditorStyle::BottomBar;

    m_TabBar = bottomBar->Add<UIHStack>();
    m_TabBar->Anchor = Vec2(0.0f);
    m_TabBar->Pivot = Vec2(0.0f);
    m_TabBar->Position = Vec2(0.0f);
    m_TabBar->Size = { 1.0_rel - 220.0_px, 1.0_rel };
    m_TabBar->Padding = UIPadding(4.0f, 3.0f);
    m_TabBar->Gap = 2.0f;

    UILabel* stats = bottomBar->Add<UILabel>();
    stats->FontSize = EditorStyle::FontSize;
    stats->Color = EditorStyle::TextDim;
    stats->HAlign = UIHAlign::Right;
    stats->VAlign = UIVAlign::Middle;
    stats->Anchor = Vec2(1.0f, 0.0f);
    stats->Pivot = Vec2(1.0f, 0.0f);
    stats->Position = Vec2(-8.0f, 0.0f);
    stats->Size = { 200.0_px, 1.0_rel };
    stats->Bind = [this, stats] {
        const double ms = m_FrameSeconds * 1000.0;
        String text = "FPS: " + std::to_string(m_FrameSeconds > 0.0 ? (int)std::lround(1.0 / m_FrameSeconds) : 0);
        String msText = std::to_string(ms);
        text += "   " + msText.substr(0, msText.find('.') + 3) + "ms";
        stats->Text = text;
    };
}

void EditorWindow::RegisterTab(MajorTab* InTab) {
    m_OpenTabs.Add(InTab);
    ActivateTab(InTab);
}

void EditorWindow::ActivateTab(MajorTab* InTab) {
    m_ActiveTab = InTab;
    for (MajorTab* tab : m_OpenTabs) {
        tab->SetEnabled(tab == InTab);
    }
    RebuildToolBar();
    RebuildTabBar();
}

void EditorWindow::RebuildToolBar() {
    ClearChildren(m_ToolBarContent);
    if (m_ActiveTab) {
        m_ActiveTab->BuildToolBar(*m_ToolBarContent);
    }
}

void EditorWindow::RebuildTabBar() {
    ClearChildren(m_TabBar);
    Font* font = UINode::GetDefaultFont();
    for (MajorTab* tab : m_OpenTabs) {
        const bool active = (tab == m_ActiveTab);
        UIButton& button = UI::Button(*m_TabBar, tab->GetTabTitle(), [this, tab] { ActivateTab(tab); });
        const float width = font ? font->MeasureText(tab->GetTabTitle(), EditorStyle::FontSize).x + 28.0f : 96.0f;
        button.Size = { UIValue(width), 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
        button.NormalColor = active ? EditorStyle::TabActive : EditorStyle::BottomBar;
        button.HoverColor = active ? EditorStyle::TabActive : EditorStyle::TabHover;
        button.PressedColor = EditorStyle::ButtonPressed;
        if (Node* child = button.GetChildByClass(UILabel::StaticClass())) {
            child->As<UILabel>()->Color = active ? EditorStyle::TextBright : EditorStyle::TextDim;
        }
    }
}

bool EditorWindow::HitTestTitleBar(const Vec2& InPoint) const {
    if (!m_TitleBar || !m_TitleBar->GetGeometry().Contains(InPoint)) {
        return false;
    }
    for (UINode* button : m_TitleBarButtons) {
        if (button->GetGeometry().Contains(InPoint)) {
            return false;
        }
    }
    return true;
}
