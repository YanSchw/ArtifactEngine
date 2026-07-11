#pragma once
#include "Window.h"
#include "GameFramework/UINode.h"
#include "EditorWindow.gen.h"

class UICanvas;
class UIStack;
class UIQuad;
class MajorTab;

/** The editor's main window. */
class EditorWindow : public Window {
public:
    ARTIFACT_CLASS();
protected:
    EditorWindow(const WindowParams& InParams);
public:
    virtual ~EditorWindow();

    static SharedObjectPtr<EditorWindow> Create(WindowParams InParams);

    UICanvas* GetCanvas() const { return m_Canvas; }

    template<typename T>
    T* OpenTab() {
        T* tab = m_TabHost->Add<T>();
        RegisterTab(tab);
        return tab;
    }
    void ActivateTab(MajorTab* InTab);
    MajorTab* GetActiveTab() const { return m_ActiveTab; }
    const Array<MajorTab*>& GetOpenTabs() const { return m_OpenTabs; }

    void SetFrameSeconds(double InSeconds) { m_FrameSeconds = InSeconds; }

    virtual bool HitTestTitleBar(const Vec2& InPoint) const override;

private:
    void BuildChrome();
    void RegisterTab(MajorTab* InTab);
    void RebuildToolBar();
    void RebuildTabBar();

    UICanvas* m_Canvas = nullptr;
    UIQuad* m_TitleBar = nullptr;
    UIStack* m_ToolBarContent = nullptr;
    UINode* m_TabHost = nullptr;
    UIStack* m_TabBar = nullptr;
    Array<UINode*> m_TitleBarButtons;

    Array<MajorTab*> m_OpenTabs;
    MajorTab* m_ActiveTab = nullptr;
    double m_FrameSeconds = 0.0;
};
