#pragma once
#include "GameFramework/UINode.h"
#include "Object/Pointer.h"
#include "Tabs/OutlinerTab.h"
#include "OutlinerRow.gen.h"

class UISvg;
class UILabel;
class UITextArea;

/** A single hierarchy row. A row is a reusable slot: it binds to whatever node currently
 *  sits at its RowIndex in the owner's visible list. */
class OutlinerRow : public UINode {
public:
    ARTIFACT_CLASS();

    static constexpr float RowHeight = 22.0f;
    static constexpr float IndentStep = 14.0f;
    /** Fixed left column holding the enable/disable eye; the tree indents to its right. */
    static constexpr float EyeColumnWidth = 18.0f;
    /** Fixed right column showing the node's class name. */
    static constexpr float TypeColumnWidth = 110.0f;

    OutlinerRow();

    OutlinerTab* Owner = nullptr;
    int RowIndex = 0;

    Node* GetBoundNode() const { return m_Node.Get(); }

    OutlinerTab::DropMode HitZone(const Vec2& InScreenCursor) const;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void Refresh();

    WeakObjectPtr<Node> m_Node;
    WeakObjectPtr<Node> m_RenameTarget;
    UISvg* m_Eye = nullptr;
    UISvg* m_Arrow = nullptr;
    UISvg* m_Icon = nullptr;
    UILabel* m_Label = nullptr;
    UILabel* m_TypeLabel = nullptr;
    UITextArea* m_RenameField = nullptr;

    Vec2 m_PressPos = Vec2(0.0f);
    bool m_Dragging = false;
    float m_DoubleClickTimer = 0.0f;
};
