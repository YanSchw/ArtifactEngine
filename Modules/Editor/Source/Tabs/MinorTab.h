#pragma once
#include "GameFramework/UINode.h"
#include "GameFramework/World.h"
#include "Object/Pointer.h"
#include "MinorTab.gen.h"

class UIDockNode;

/** Base class for dockable editor panels (Outliner, Details, Viewport, ...). A MinorTab lives
 *  in a UIDockNode leaf inside a MajorTab's dock area; subclasses build their content as
 *  children in their constructor. */
class MinorTab : public UINode {
public:
    ARTIFACT_CLASS();

    MinorTab();

    virtual String GetTabTitle() const { return "Tab"; }
    /** Transparent panels paint no background, leaving a hole in the editor chrome through
     *  which the 3D scene rendered behind the UI shows. */
    virtual bool HasTransparentBackground() const { return false; }

    World* GetEditedWorld() const { return m_EditedWorld.Get(); }
    void SetEditedWorld(World* InWorld) { m_EditedWorld = InWorld; }

    UIDockNode* GetDockNode() const;

private:
    WeakObjectPtr<World> m_EditedWorld;
};
