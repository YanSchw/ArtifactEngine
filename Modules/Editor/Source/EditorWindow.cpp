#include "EditorWindow.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorStyle.h"
#include "UI/UIMajorTabHandle.h"
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
    : ThemedWindow(InParams) {
    BuildEditorChrome();
}

EditorWindow::~EditorWindow() {
}

SharedObjectPtr<EditorWindow> EditorWindow::Create(WindowParams InParams) {
    InParams.EditorStyle = true;
    SharedObjectPtr<EditorWindow> window(new EditorWindow(InParams));
    ThemedWindow::RegisterWindow(window);
    return window;
}

void EditorWindow::BuildEditorChrome() {
    UIVStack* column = GetContentRoot()->Add<UIVStack>();
    column->Fill();
    // Chrome rebuilds happen in the bind pass, before layout, so tab switches and moves
    // never delete nodes that are still routing this frame's input.
    column->Bind = [this] {
        if (m_ChromeDirty) {
            m_ChromeDirty = false;
            RebuildToolBar();
            RebuildTabBar();
        }
    };

    UIQuad* toolBar = column->Add<UIQuad>();
    toolBar->Size = { 1.0_rel, UIValue(EditorStyle::ToolBarHeight) };
    toolBar->Color = EditorStyle::ToolBar;
    m_ToolBarContent = toolBar->Add<UIHStack>();
    m_ToolBarContent->Fill();
    m_ToolBarContent->Padding = UIPadding(6.0f, 6.0f);
    m_ToolBarContent->Gap = 6.0f;

    m_TabHost = column->Add<UINode>();
    m_TabHost->Size = { 1.0_rel, 1.0_rel };

    UIQuad* bottomBar = column->Add<UIQuad>();
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
    InTab->SetOwnerWindow(this);
    ActivateTab(InTab);
}

void EditorWindow::ActivateTab(MajorTab* InTab) {
    if (InTab && !m_OpenTabs.Contains(InTab)) {
        return;
    }
    m_ActiveTab = InTab;
    for (MajorTab* tab : m_OpenTabs) {
        const bool active = (tab == InTab);
        tab->SetEnabled(active);
        tab->SetFloatingWindowsVisible(active);
    }
    m_ChromeDirty = true;
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
        UIMajorTabHandle* handle = m_TabBar->Add<UIMajorTabHandle>();
        handle->Tab = tab;
        handle->OwnerWindow = this;
        handle->SetCaption(tab->GetTabTitle());
        handle->Clicked = [this, tab] { ActivateTab(tab); };
        const float width = font ? font->MeasureText(tab->GetTabTitle(), EditorStyle::FontSize).x + 28.0f : 96.0f;
        handle->Size = { UIValue(width), 1.0_rel };
        EditorStyle::ApplyButtonStyle(*handle);
        handle->NormalColor = active ? EditorStyle::TabActive : EditorStyle::BottomBar;
        handle->HoverColor = active ? EditorStyle::TabActive : EditorStyle::TabHover;
        handle->PressedColor = EditorStyle::ButtonPressed;
        if (Node* child = handle->GetChildByClass(UILabel::StaticClass())) {
            child->As<UILabel>()->Color = active ? EditorStyle::TextBright : EditorStyle::TextDim;
        }
    }
}

void EditorWindow::BeginTabHandleDrag(MajorTab* InTab) {
    (void)InTab;
    m_DraggingTabHandle = true;
}

void EditorWindow::UpdateTabHandleDrag(const Vec2& InCursorPos) {
    m_TabHandleDragCursor = InCursorPos;
}

void EditorWindow::EndTabHandleDrag(MajorTab* InTab) {
    if (!m_DraggingTabHandle) {
        return;
    }
    m_DraggingTabHandle = false;

    const Vec2 screenPos = GetPosition() + m_TabHandleDragCursor;
    if (ContainsScreenPoint(screenPos)) {
        return;
    }

    if (ThemedWindow* hit = ThemedWindow::FindWindowAtScreenPoint(screenPos, this)) {
        if (EditorWindow* other = hit->As<EditorWindow>()) {
            MoveTab(InTab, this, other);
        }
        return;
    }

    // Tearing the sole tab off into a new window would just move this window's content around
    // for nothing — leave it docked.
    if (m_OpenTabs.Size() <= 1) {
        return;
    }

    WindowParams params;
    params.Title = "Artifact Editor";
    params.Width = 1100;
    params.Height = 650;
    SharedObjectPtr<EditorWindow> spawned = EditorWindow::Create(params);
    spawned->SetPosition(screenPos - Vec2(120.0f, 12.0f));
    MoveTab(InTab, this, spawned.Get());
}

void EditorWindow::MoveTab(MajorTab* InTab, EditorWindow* InFrom, EditorWindow* InTo) {
    if (!InTab || !InFrom || !InTo || InFrom == InTo) {
        return;
    }
    InFrom->m_OpenTabs.Remove(InTab);
    InTab->SetParent(InTo->m_TabHost);
    InTo->RegisterTab(InTab);
    InFrom->ActivateTab(InFrom->m_OpenTabs.IsEmpty() ? nullptr : InFrom->m_OpenTabs[0]);
    if (InFrom->m_OpenTabs.IsEmpty()) {
        InFrom->Close();
    }
}

