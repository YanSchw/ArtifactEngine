#pragma once
#include "GameFramework/UINode.h"
#include "MajorTab.gen.h"

class UIDockArea;

/** A top-level editor document — the heart of the editor. A MajorTab edits one thing (a scene,
 *  a mesh, a texture, ...), hosts a dock area of MinorTabs as its workspace, fills the editor
 *  tool bar while active, and is listed in the bottom tab bar of the EditorWindow. */
class MajorTab : public UINode {
public:
    ARTIFACT_CLASS();

    MajorTab();

    virtual String GetTabTitle() const { return "Tab"; }
    virtual void BuildToolBar(UINode& InToolBar) { (void)InToolBar; }

    UIDockArea* GetDockArea() const { return m_DockArea; }

private:
    UIDockArea* m_DockArea = nullptr;
};
