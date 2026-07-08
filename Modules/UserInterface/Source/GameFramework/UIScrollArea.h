#pragma once
#include "UINode.h"
#include "UIScrollArea.gen.h"

class UIScrollArea : public UINode {
public:
    ARTIFACT_CLASS();

    UIScrollArea();

    Vec2 ScrollOffset = Vec2(0.0f);
    float ScrollSpeed = 40.0f;
    Vec4 ScrollbarColor = Vec4(1.0f, 1.0f, 1.0f, 0.25f);
    float ScrollbarWidth = 4.0f;

    const Vec2& GetContentSize() const { return m_ContentSize; }

    virtual bool OnScroll(const Vec2& InDelta) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;

protected:
    virtual void LayoutChildren(const UIRectF& InContent) override;
    virtual void PaintOverlay(UIDrawList& OutDrawList) override;

private:
    void ClampScrollOffset();
    UIRectF ComputeVerticalThumbRect(const UIRectF& InContent) const;
    UIRectF ComputeHorizontalThumbRect(const UIRectF& InContent) const;
    /** Screen point -> local position along InAxis (0=x, 1=y) by parametrizing the cursor along
     *  the content rect's projected edge — avoids inverting the full canvas projection. */
    float ScreenPosToLocalAxis(const Vec2& InScreenPos, const UIRectF& InContent, int InAxis) const;

    enum class EDragTarget : uint8_t { None, VerticalThumb, HorizontalThumb };
    EDragTarget m_DragTarget = EDragTarget::None;
    float m_DragGrabOffset = 0.0f;

    Vec2 m_ContentSize = Vec2(0.0f);
};
