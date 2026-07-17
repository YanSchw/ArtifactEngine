#pragma once
#include "GameFramework/UINode.h"
#include "GameFramework/World.h"
#include "Object/Pointer.h"
#include "MajorTab.gen.h"

class UIDockArea;
class MinorTab;
class MinorTabStandaloneWindow;
class EditorWindow;

/** A top-level editor document — the heart of the editor. A MajorTab edits one thing (a scene,
 *  a mesh, a texture, ...), hosts a dock area of MinorTabs as its workspace, fills the editor
 *  tool bar while active, and is listed in the bottom tab bar of its EditorWindow. */
class MajorTab : public UINode {
public:
    ARTIFACT_CLASS();

    MajorTab();
    virtual ~MajorTab();

    virtual String GetTabTitle() const { return "Tab"; }
    virtual void BuildToolBar(UINode& InToolBar) { (void)InToolBar; }

    World* GetEditedWorld() const { return m_World.Get(); }

    UIDockArea* GetDockArea() const { return m_DockArea; }

    void SetOwnerWindow(EditorWindow* InWindow) { m_OwnerWindow = InWindow; }
    EditorWindow* GetOwnerWindow() const { return m_OwnerWindow; }

    void FloatTab(MinorTab* InTab, const Vec2& InScreenPos);
    void ReDockFloatingTab(MinorTabStandaloneWindow* InWindow);
    void SetFloatingWindowsVisible(bool InVisible);

protected:
    void SetEditedWorld(World* InWorld);

private:
    UIDockArea* m_DockArea = nullptr;
    EditorWindow* m_OwnerWindow = nullptr;
    Array<WeakObjectPtr<MinorTabStandaloneWindow>> m_FloatingWindows;
    SharedObjectPtr<World> m_World;
};
