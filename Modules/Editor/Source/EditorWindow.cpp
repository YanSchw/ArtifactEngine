#include "EditorWindow.h"
#include "Tabs/MajorTab.h"
#include "Tabs/SceneEditorTab.h"
#include "Tabs/BlueprintEditorTab.h"
#include "Assets/Scene.h"
#include "Assets/Blueprint.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIContextMenu.h"
#include "UI/UIMajorTabHandle.h"
#include "HeroTools/HeroTool.h"
#include "HeroTools/ContentDrawer.h"
#include "HeroTools/ConsoleTool.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UILabel.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/KeyCodes.h"
#include "Assets/Font.h"
#include <algorithm>
#include <cmath>
#include <string>

static void ClearChildren(UINode* InNode) {
    while (InNode->HasChildren()) {
        delete InNode->GetChild(0);
    }
}

#if defined(AE_PLATFORM_MACOS)
static const String s_CommandKey = "Cmd";
#else
static const String s_CommandKey = "Ctrl";
#endif

EditorWindow::EditorWindow(const WindowParams& InParams)
    : ThemedWindow(InParams) {
    RegisterHeroTools();
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

void EditorWindow::RegisterHeroTools() {
    SharedObjectPtr<ContentDrawer> contentDrawer = Object::Create<ContentDrawer>();
    contentDrawer->SetOwnerWindow(this);
    m_ContentDrawerTool = contentDrawer.Get();
    m_HeroTools.Add(contentDrawer);

    SharedObjectPtr<ConsoleTool> console = Object::Create<ConsoleTool>();
    console->SetOwnerWindow(this);
    m_ConsoleTool = console.Get();
    m_HeroTools.Add(console);
}

void EditorWindow::BuildTitleBarMenus() {
    UIHStack* row = m_TitleBar->Add<UIHStack>();
    row->Anchor = row->Pivot = Vec2(0.0f, 0.0f);
#if defined(AE_PLATFORM_MACOS)
    row->Position = Vec2(EditorStyle::MacTrafficLightInset, 0.0f);
#else
    row->Position = Vec2(6.0f, 0.0f);
#endif
    row->Size = { 400.0_px, 1.0_rel };

    AddTitleBarMenu(*row, "File", [this](UIMenuModel& OutMenu) { BuildFileMenu(OutMenu); });
    AddTitleBarMenu(*row, "Window", [this](UIMenuModel& OutMenu) { BuildWindowMenu(OutMenu); });
}

void EditorWindow::AddTitleBarMenu(UINode& InRow, const String& InTitle, std::function<void(UIMenuModel&)> InBuild) {
    UIButton* button = InRow.Add<UIButton>();
    button->SetCaption(InTitle);
    button->Size = { 60.0_px, 1.0_rel };
    button->NormalColor = Vec4(0.0f);
    button->HoverColor = EditorStyle::CaptionHover;
    button->PressedColor = EditorStyle::CaptionHover;
    button->Cursor = CursorIcon::Arrow;
    if (Node* child = button->GetChildByClass(UILabel::StaticClass())) {
        UILabel* label = child->As<UILabel>();
        label->FontSize = EditorStyle::FontSize;
        label->Color = EditorStyle::Text;
    }
    // The chrome is built before the default font exists, so the width follows the caption.
    button->Bind = [button, InTitle] {
        if (Font* font = UINode::GetDefaultFont()) {
            button->Size = { UIValue(font->GetTextWidth(InTitle, EditorStyle::FontSize) + 22.0f), 1.0_rel };
        }
    };
    button->Clicked = [button, build = std::move(InBuild)] {
        UIMenuModel menu;
        build(menu);
        UIContextMenu::OpenUnder(*button, menu);
    };

    // Title-bar hit-testing hands everything it does not know about to the OS as a drag region.
    m_TitleBarButtons.Add(button);
}

void EditorWindow::BuildFileMenu(UIMenuModel& OutMenu) {
    OutMenu.Section("Editor");
    OutMenu.Item("New Scene Tab", [this] { OpenTab<SceneEditorTab>(); }).Icon(EditorIcons::Level());
    OutMenu.Item("New Window", [this] {
        WindowParams params;
        params.Title = "Artifact Editor";
        params.Width = 1100;
        params.Height = 650;
        SharedObjectPtr<EditorWindow> spawned = EditorWindow::Create(params);
        spawned->SetPosition(GetPosition() + Vec2(40.0f, 40.0f));
        spawned->OpenTab<SceneEditorTab>();
    }).Icon(EditorIcons::Document());
    OutMenu.Separator();
    OutMenu.Item("Close Window", [this] { Close(); });
}

void EditorWindow::BuildWindowMenu(UIMenuModel& OutMenu) {
    OutMenu.Section("Hero Tools");
    for (const SharedObjectPtr<HeroTool>& tool : m_HeroTools) {
        HeroTool* toolPtr = tool.Get();
        String shortcut;
        if (toolPtr == m_ContentDrawerTool) {
            shortcut = s_CommandKey + "+Space";
        } else if (toolPtr == m_ConsoleTool) {
            shortcut = s_CommandKey + "+.";
        }
        OutMenu.Item(toolPtr->GetTitle(), [this, toolPtr] { ToggleHeroTool(toolPtr); })
               .Shortcut(shortcut)
               .Checked(IsHeroToolOpen(toolPtr));
    }

    OutMenu.Section("Tabs");
    for (MajorTab* tab : m_OpenTabs) {
        OutMenu.Item(tab->GetTabTitle(), [this, tab] { ActivateTab(tab); })
               .Checked(tab == m_ActiveTab);
    }
}

void EditorWindow::BuildEditorChrome() {
    BuildTitleBarMenus();

    UIVStack* column = GetContentRoot()->Add<UIVStack>();
    column->Fill();
    // Chrome rebuilds happen in the bind pass, before layout, so tab switches and moves
    // never delete nodes that are still routing this frame's input.
    column->Bind = [this] {
        HandleShortcuts();
        if (m_ChromeDirty) {
            m_ChromeDirty = false;
            RebuildToolBar();
            RebuildBottomBar();
        }
        if (m_DrawerDirty) {
            m_DrawerDirty = false;
            RebuildDrawerBody();
        }
        if (m_ActiveHeroTool) {
            m_ActiveHeroTool->Tick((float)m_FrameSeconds);
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

    m_BottomRow = bottomBar->Add<UIHStack>();
    m_BottomRow->Fill();
    m_BottomRow->Padding = UIPadding(4.0f, 3.0f);
    m_BottomRow->Gap = 4.0f;

    // The drawer overlay sits above the bottom bar and covers the workspace; it lives in the window
    // chrome rather than in a MajorTab, so it stays available while switching tabs.
    m_DrawerHost = GetContentRoot()->Add<UINode>();
    m_DrawerHost->Anchor = m_DrawerHost->Pivot = Vec2(0.0f, 1.0f);
    m_DrawerHost->Position = Vec2(0.0f, -EditorStyle::BottomBarHeight);
    m_DrawerHost->Size = { 1.0_rel, 0.42_rel };
    m_DrawerHost->ClipChildren = true;
    m_DrawerHost->SetEnabled(false);

    UIQuad* drawerBg = m_DrawerHost->Add<UIQuad>();
    drawerBg->Fill();
    drawerBg->Color = EditorStyle::PanelDark;

    UIQuad* drawerTopLine = m_DrawerHost->Add<UIQuad>();
    drawerTopLine->Anchor = drawerTopLine->Pivot = Vec2(0.0f, 0.0f);
    drawerTopLine->Position = Vec2(0.0f, 0.0f);
    drawerTopLine->Size = { 1.0_rel, UIValue(1.0f) };
    drawerTopLine->Color = EditorStyle::Accent;

    m_DrawerBody = m_DrawerHost->Add<UINode>();
    m_DrawerBody->Anchor = m_DrawerBody->Pivot = Vec2(0.0f, 0.0f);
    m_DrawerBody->Position = Vec2(0.0f, 1.0f);
    m_DrawerBody->Size = { 1.0_rel, 1.0_rel - 1.0_px };

    m_ChromeDirty = true;
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

MajorTab* EditorWindow::OpenAssetEditor(Asset* InAsset) {
    if (!InAsset) {
        return nullptr;
    }

    for (MajorTab* tab : m_OpenTabs) {
        if (tab->GetEditedAsset() == InAsset) {
            ActivateTab(tab);
            return tab;
        }
    }

    if (Scene* scene = Cast<Scene>(InAsset)) {
        SceneEditorTab* tab = OpenTab<SceneEditorTab>();
        tab->OpenScene(scene);
        return tab;
    }
    if (Blueprint* blueprint = Cast<Blueprint>(InAsset)) {
        BlueprintEditorTab* tab = OpenTab<BlueprintEditorTab>();
        tab->OpenBlueprint(blueprint);
        return tab;
    }

    AE_INFO("No editor for asset '{0}' ({1})", InAsset->GetDisplayName(), InAsset->GetClass().Name);
    return nullptr;
}

void EditorWindow::CloseTab(MajorTab* InTab) {
    if (!InTab || !m_OpenTabs.Contains(InTab)) {
        return;
    }
    const bool wasActive = (m_ActiveTab == InTab);
    const int32_t index = m_OpenTabs.IndexOf(InTab);
    m_OpenTabs.Remove(InTab);
    if (UICanvas* canvas = InTab->GetCanvas()) {
        canvas->DestroyDeferred(InTab);
    } else {
        delete InTab;
    }

    if (m_OpenTabs.IsEmpty()) {
        ActivateTab(nullptr);
        Close();
        return;
    }
    if (wasActive) {
        ActivateTab(m_OpenTabs[std::min(index, m_OpenTabs.Size() - 1)]);
    } else {
        m_ChromeDirty = true;
    }
}

void EditorWindow::RebuildToolBar() {
    ClearChildren(m_ToolBarContent);
    if (m_ActiveTab) {
        m_ActiveTab->BuildToolBar(*m_ToolBarContent);
    }
}

void EditorWindow::RebuildBottomBar() {
    ClearChildren(m_BottomRow);

    for (const SharedObjectPtr<HeroTool>& tool : m_HeroTools) {
        if (tool->HasStatusButton() && !tool->IsRightAligned()) {
            AddHeroToolButton(*m_BottomRow, tool.Get());
        }
    }

    AddLogStatusButton(*m_BottomRow);

    m_TabBar = m_BottomRow->Add<UIHStack>();
    m_TabBar->Size = { 1.0_rel, 1.0_rel };
    m_TabBar->Gap = 2.0f;

    for (const SharedObjectPtr<HeroTool>& tool : m_HeroTools) {
        if (tool->HasStatusButton() && tool->IsRightAligned()) {
            AddHeroToolButton(*m_BottomRow, tool.Get());
        }
    }

    UILabel* stats = m_BottomRow->Add<UILabel>();
    stats->Size = { 200.0_px, 1.0_rel };
    stats->FontSize = EditorStyle::FontSize;
    stats->Color = EditorStyle::TextDim;
    stats->HAlign = UIHAlign::Right;
    stats->VAlign = UIVAlign::Middle;
    stats->Padding = UIPadding(0.0f, 0.0f, 8.0f, 0.0f);
    stats->Bind = [this, stats] {
        const double ms = m_FrameSeconds * 1000.0;
        String text = "FPS: " + std::to_string(m_FrameSeconds > 0.0 ? (int)std::lround(1.0 / m_FrameSeconds) : 0);
        String msText = std::to_string(ms);
        text += "   " + msText.substr(0, msText.find('.') + 3) + "ms";
        stats->Text = text;
    };

    RebuildTabBar();
}

void EditorWindow::RebuildTabBar() {
    ClearChildren(m_TabBar);
    for (MajorTab* tab : m_OpenTabs) {
        const bool active = (tab == m_ActiveTab);
        UIMajorTabHandle* handle = m_TabBar->Add<UIMajorTabHandle>();
        handle->Tab = tab;
        handle->OwnerWindow = this;
        handle->SetContent(tab->GetTabTitle(), tab->GetTabIcon());
        handle->SetActive(active);
        handle->Clicked = [this, tab] { ActivateTab(tab); };
    }
}

void EditorWindow::AddHeroToolButton(UINode& InRow, HeroTool* InTool) {
    UIButton* button = InRow.Add<UIButton>();
    button->Size = { UIValue(InTool->GetStatusButtonWidth()), 1.0_rel };
    button->NormalColor = EditorStyle::BottomBar;
    button->HoverColor = EditorStyle::TabHover;
    button->PressedColor = EditorStyle::ButtonPressed;

    HeroTool* toolPtr = InTool;
    button->Clicked = [this, toolPtr] { ToggleHeroTool(toolPtr); };
    button->Bind = [this, button, toolPtr] {
        button->NormalColor = IsHeroToolOpen(toolPtr) ? EditorStyle::Panel : EditorStyle::BottomBar;
    };

    if (InTool->BuildStatusWidget(*button)) {
        return;
    }

    UISvg* chevron = button->Add<UISvg>();
    chevron->Anchor = chevron->Pivot = Vec2(0.0f, 0.5f);
    chevron->Position = Vec2(10.0f, 0.0f);
    chevron->Size = Vec2(11.0f, 11.0f);
    chevron->Tint = EditorStyle::TextDim;
    chevron->Image = EditorIcons::ArrowUp();
    chevron->Bind = [this, chevron, toolPtr] {
        chevron->Image = IsHeroToolOpen(toolPtr) ? EditorIcons::ArrowDown() : EditorIcons::ArrowUp();
    };

    UILabel* label = button->Add<UILabel>();
    label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
    label->Position = Vec2(28.0f, 0.0f);
    label->Size = { 1.0_rel - 36.0_px, 1.0_rel };
    label->FontSize = EditorStyle::FontSize;
    label->Color = EditorStyle::Text;
    label->VAlign = UIVAlign::Middle;
    label->Text = InTool->GetTitle();
}

void EditorWindow::AddLogStatusButton(UINode& InRow) {
    ConsoleTool* console = m_ConsoleTool ? m_ConsoleTool->As<ConsoleTool>() : nullptr;
    if (!console) {
        return;
    }

    UIButton* button = InRow.Add<UIButton>();
    button->Size = { UIValue(ConsoleTool::GetLogStatusWidth()), 1.0_rel };
    button->NormalColor = EditorStyle::BottomBar;
    button->HoverColor = EditorStyle::TabHover;
    button->PressedColor = EditorStyle::ButtonPressed;
    button->Clicked = [this, console] { ToggleHeroTool(console); };
    button->Bind = [this, button, console] {
        button->NormalColor = IsHeroToolOpen(console) ? EditorStyle::Panel : EditorStyle::BottomBar;
    };

    console->BuildLogStatus(*button);
}

void EditorWindow::RebuildDrawerBody() {
    ClearChildren(m_DrawerBody);
    if (m_ActiveHeroTool) {
        m_ActiveHeroTool->BuildDrawer(*m_DrawerBody);
    }
}

void EditorWindow::ToggleHeroTool(HeroTool* InTool) {
    if (m_ActiveHeroTool == InTool) {
        CloseHeroTool();
    } else {
        OpenHeroTool(InTool);
    }
}

void EditorWindow::OpenHeroTool(HeroTool* InTool) {
    m_ActiveHeroTool = InTool;
    m_DrawerHost->SetEnabled(InTool != nullptr);
    m_DrawerDirty = true;
}

void EditorWindow::CloseHeroTool() {
    m_ActiveHeroTool = nullptr;
    m_DrawerHost->SetEnabled(false);
    ClearChildren(m_DrawerBody);
}

void EditorWindow::HandleShortcuts() {
    if (!IsFocused()) {
        return;
    }
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (!keyboard) {
        return;
    }
    const bool ctrl = keyboard->IsPressed(KeyCode::LeftControl) || keyboard->IsPressed(KeyCode::RightControl)
                   || keyboard->IsPressed(KeyCode::LeftSuper) || keyboard->IsPressed(KeyCode::RightSuper);
    if (ctrl && keyboard->IsDown(KeyCode::Space)) {
        ToggleHeroTool(m_ContentDrawerTool);
    } else if (ctrl && keyboard->IsDown(KeyCode::Period)) {
        ToggleHeroTool(m_ConsoleTool);
    } else if (keyboard->IsDown(KeyCode::Escape) && m_ActiveHeroTool) {
        CloseHeroTool();
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
