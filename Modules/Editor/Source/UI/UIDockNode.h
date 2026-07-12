#pragma once
#include "GameFramework/UINode.h"
#include "UIDockNode.gen.h"

class MinorTab;
class UIDockArea;
class UIDockSplitter;

enum class UIDockSlot : uint8_t { Center = 0, Left, Right, Top, Bottom };

/** One node of a dock tree. A leaf hosts MinorTabs behind a tab header row; a split holds two
 *  child dock nodes separated by a draggable UIDockSplitter. */
class UIDockNode : public UINode {
public:
    ARTIFACT_CLASS();

    UIDockNode();

    bool IsLeaf() const { return m_ChildA == nullptr; }

    void AddTab(MinorTab* InTab, bool InActivate = true);
    void RemoveTab(MinorTab* InTab);
    const Array<MinorTab*>& GetTabs() const { return m_Tabs; }
    MinorTab* GetActiveTab() const { return m_ActiveTab; }
    void SetActiveTab(MinorTab* InTab);

    UIDockNode* GetChildA() const { return m_ChildA; }
    UIDockNode* GetChildB() const { return m_ChildB; }

    /** Turns this node into a split: its current content moves into a new child, InInserted
     *  becomes the other child on the InSlot side with InInsertedShare of the space. */
    void BecomeSplit(UIDockSlot InSlot, UIDockNode* InInserted, float InInsertedShare);
    /** Turns this split back into a copy of InKeep (one of its children), deleting the shells. */
    void AbsorbChild(UIDockNode* InKeep);
    void AdjustSplit(const Vec2& InCursorDelta);

    UIDockNode* FindLeafAt(const Vec2& InPoint);
    UIDockNode* FindFirstLeaf() { return IsLeaf() ? this : m_ChildA->FindFirstLeaf(); }
    UIRectF GetHeaderRect() const;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;

protected:
    virtual void LayoutChildren(const UIRectF& InContent) override;

private:
    UIDockArea* GetArea() const;
    float TabHandleWidth(int InTabIndex) const;
    UIRectF TabHandleRect(int InTabIndex) const;
    int TabHandleIndexAt(const Vec2& InPoint) const;

    Array<MinorTab*> m_Tabs;
    MinorTab* m_ActiveTab = nullptr;

    UIDockNode* m_ChildA = nullptr;  // left / top
    UIDockNode* m_ChildB = nullptr;  // right / bottom
    UIDockSplitter* m_Splitter = nullptr;
    bool m_SplitHorizontal = true;
    float m_SplitRatio = 0.5f;

    int m_PressedTabIndex = -1;
    Vec2 m_PressPosition = Vec2(0.0f);
    Vec2 m_LastCursor = Vec2(0.0f);
};
