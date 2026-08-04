#pragma once
#include "ThemedWindow.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include <functional>
#include "EditorWindow.gen.h"

class UIStack;
class UINode;
class MajorTab;
class HeroTool;
class UIMenuModel;

/** The editor's main window. */
class EditorWindow : public ThemedWindow {
public:
    ARTIFACT_CLASS();
protected:
    EditorWindow(const WindowParams& InParams);
public:
    virtual ~EditorWindow();

    static SharedObjectPtr<EditorWindow> Create(WindowParams InParams);

    template<typename T>
    T* OpenTab() {
        T* tab = m_TabHost->Add<T>();
        RegisterTab(tab);
        return tab;
    }
    void ActivateTab(MajorTab* InTab);
    void CloseTab(MajorTab* InTab);
    MajorTab* OpenAssetEditor(class Asset* InAsset);
    MajorTab* GetActiveTab() const { return m_ActiveTab; }
    const Array<MajorTab*>& GetOpenTabs() const { return m_OpenTabs; }

    void BeginTabHandleDrag(MajorTab* InTab);
    void UpdateTabHandleDrag(const Vec2& InCursorPos);
    void EndTabHandleDrag(MajorTab* InTab);
    bool IsDraggingTabHandle() const { return m_DraggingTabHandle; }

    static void MoveTab(MajorTab* InTab, EditorWindow* InFrom, EditorWindow* InTo);

    void MarkChromeDirty() { m_ChromeDirty = true; }
    static void MarkAllChromeDirty();

    void ToggleHeroTool(HeroTool* InTool);
    void OpenHeroTool(HeroTool* InTool);
    void CloseHeroTool();
    bool IsHeroToolOpen(const HeroTool* InTool) const { return m_ActiveHeroTool == InTool; }

private:
    void BuildEditorChrome();
    void BuildTitleBarMenus();
    void AddTitleBarMenu(UINode& InRow, const String& InTitle, std::function<void(UIMenuModel&)> InBuild);
    void BuildFileMenu(UIMenuModel& OutMenu);
    void BuildWindowMenu(UIMenuModel& OutMenu);
    void RegisterHeroTools();
    void RegisterTab(MajorTab* InTab);
    void RebuildToolBar();
    void RebuildBottomBar();
    void RebuildTabBar();
    void RebuildDrawerBody();
    void AddHeroToolButton(UINode& InRow, HeroTool* InTool);
    void AddLogStatusButton(UINode& InRow);
    void HandleShortcuts();

    UIStack* m_ToolBarContent = nullptr;
    UINode* m_TabHost = nullptr;
    UIStack* m_BottomRow = nullptr;
    UIStack* m_TabBar = nullptr;
    UINode* m_DrawerHost = nullptr;
    UINode* m_DrawerBody = nullptr;

    Array<MajorTab*> m_OpenTabs;
    MajorTab* m_ActiveTab = nullptr;
    bool m_ChromeDirty = false;

    Array<SharedObjectPtr<HeroTool>> m_HeroTools;
    HeroTool* m_ContentDrawerTool = nullptr;
    HeroTool* m_ConsoleTool = nullptr;
    HeroTool* m_ActiveHeroTool = nullptr;
    bool m_DrawerDirty = false;

    bool m_DraggingTabHandle = false;
    Vec2 m_TabHandleDragCursor = Vec2(0.0f);
};
