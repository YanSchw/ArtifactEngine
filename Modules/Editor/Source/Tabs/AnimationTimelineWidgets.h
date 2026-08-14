#pragma once
#include "GameFramework/UINode.h"
#include "Object/Pointer.h"
#include "AnimationTimelineWidgets.gen.h"

class AnimationTimelineTab;
class Node;
class UILabel;
class UISvg;
struct Property;

/** Shared by every part of the timeline that maps frames to pixels. */
class AnimationTimelineStrip : public UINode {
public:
    ARTIFACT_CLASS();

    AnimationTimelineStrip();

    AnimationTimelineTab* Owner = nullptr;

    virtual bool OnScroll(const Vec2& InDelta) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

protected:
    virtual bool WheelZooms() const { return true; }
    UIRectF Strip() const;
    bool IsOverLabelColumn(const Vec2& InCursorPos) const;
    void PaintGrid(UIDrawList& OutDrawList, const Vec4& InColor);
    void PaintPlayHead(UIDrawList& OutDrawList);
    /** Dims whatever lies before frame 0 or past the last frame. */
    void PaintOutOfRange(UIDrawList& OutDrawList);

    Vec2 m_Cursor = Vec2(0.0f);
};

/** Frame numbers, tick marks and the playhead. */
class AnimationTimelineRuler : public AnimationTimelineStrip {
public:
    ARTIFACT_CLASS();

    AnimationTimelineRuler();

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
};

/** Every key of every track on one line. */
class AnimationTimelineSummary : public AnimationTimelineStrip {
public:
    ARTIFACT_CLASS();

    AnimationTimelineSummary();

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
};

/** One animated property. */
class AnimationTimelineRow : public AnimationTimelineStrip {
public:
    ARTIFACT_CLASS();

    AnimationTimelineRow();

    int32_t RowIndex = 0;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
    virtual bool OnSecondaryClick(const Vec2& InCursorPos) override;

private:
    virtual bool WheelZooms() const override { return false; }
    void Refresh();
    void SetEditor(Node* InTarget, Property* InLeaf);
    int32_t KeyAt(const Vec2& InCursorPos) const;

    UISvg* m_Expander = nullptr;
    UISvg* m_Icon = nullptr;
    UILabel* m_Label = nullptr;
    UINode* m_ValueHost = nullptr;
    WeakObjectPtr<Node> m_EditorTarget;
    Property* m_EditorLeaf = nullptr;
    int32_t m_DraggedKey = -1;
    bool m_PressedOnLabel = false;
};

/** The horizontal view window over the whole animation. */
class AnimationTimelineScrollBar : public UINode {
public:
    ARTIFACT_CLASS();

    AnimationTimelineScrollBar();

    AnimationTimelineTab* Owner = nullptr;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;

private:
    UIRectF ThumbRect() const;

    float m_GrabOffset = 0.0f;
};
