#pragma once
#include "GameFramework/UIButton.h"
#include <functional>
#include "ContentTile.gen.h"

/** A content-browser card */
class ContentTile : public UIButton {
public:
    ARTIFACT_CLASS();

    float CornerRadius = 0.0f;
    std::function<void()> DoubleClicked;
    std::function<void()> DragStarted;
    std::function<void(const Vec2&)> DragMoved;
    std::function<void()> DragEnded;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
    virtual void OnClick() override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    static constexpr float s_DoubleClickTime = 0.35f;
    static constexpr float s_DragThresholdSq = 16.0f;
    float m_DoubleClickTimer = 0.0f;
    bool m_DoublePending = false;
    Vec2 m_PressPos = Vec2(0.0f);
    bool m_Dragging = false;
};
