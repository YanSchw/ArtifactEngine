#pragma once
#include "GameFramework/UINode.h"
#include "UIDockNode.h"
#include "UIDockArea.gen.h"

class MinorTab;

/** The docking surface of a MajorTab: owns a tree of UIDockNodes, places MinorTabs into it,
 *  and orchestrates dragging tab handles between leaves. */
class UIDockArea : public UINode {
public:
    ARTIFACT_CLASS();

    UIDockArea();

    UIDockNode* GetRoot() const { return m_Root; }

    /** Docks the tab at InSlot relative to InTarget (the root when null). Non-center slots
     *  split the target, giving the new leaf InShare of its space. Returns the leaf. */
    UIDockNode* Dock(MinorTab* InTab, UIDockSlot InSlot, UIDockNode* InTarget = nullptr, float InShare = 0.3f);

    template<typename T>
    T* DockNew(UIDockSlot InSlot, UIDockNode* InTarget = nullptr, float InShare = 0.3f) {
        T* tab = Object::Create<T>();
        Dock(tab, InSlot, InTarget, InShare);
        return tab;
    }

    bool IsDraggingTab() const { return m_DraggedTab != nullptr; }
    void BeginTabDrag(MinorTab* InTab, UIDockNode* InSource, const Vec2& InCursorPos);
    void UpdateTabDrag(const Vec2& InCursorPos);
    void EndTabDrag();

protected:
    virtual void PaintOverlay(UIDrawList& OutDrawList) override;

private:
    bool ResolveDropTarget(const Vec2& InPoint, UIDockNode*& OutNode, UIDockSlot& OutSlot, UIRectF& OutPreview) const;
    void DetachTab(MinorTab* InTab, UIDockNode* InSource, UIDockNode*& InOutTarget);
    void FloatDraggedTab(MinorTab* InTab, UIDockNode* InSource);

    UIDockNode* m_Root = nullptr;
    MinorTab* m_DraggedTab = nullptr;
    UIDockNode* m_DragSource = nullptr;
    Vec2 m_DragCursor = Vec2(0.0f);
};
