#pragma once
#include "ThemedWindow.h"
#include "EditorWindow.gen.h"

class UIStack;
class MajorTab;

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
    MajorTab* GetActiveTab() const { return m_ActiveTab; }
    const Array<MajorTab*>& GetOpenTabs() const { return m_OpenTabs; }

    void BeginTabHandleDrag(MajorTab* InTab);
    void UpdateTabHandleDrag(const Vec2& InCursorPos);
    void EndTabHandleDrag(MajorTab* InTab);
    bool IsDraggingTabHandle() const { return m_DraggingTabHandle; }

    static void MoveTab(MajorTab* InTab, EditorWindow* InFrom, EditorWindow* InTo);

private:
    void BuildEditorChrome();
    void RegisterTab(MajorTab* InTab);
    void RebuildToolBar();
    void RebuildTabBar();

    UIStack* m_ToolBarContent = nullptr;
    UINode* m_TabHost = nullptr;
    UIStack* m_TabBar = nullptr;

    Array<MajorTab*> m_OpenTabs;
    MajorTab* m_ActiveTab = nullptr;
    bool m_ChromeDirty = false;

    bool m_DraggingTabHandle = false;
    Vec2 m_TabHandleDragCursor = Vec2(0.0f);
};
