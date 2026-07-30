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

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnClick() override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    static constexpr float s_DoubleClickTime = 0.35f;
    float m_DoubleClickTimer = 0.0f;
    bool m_DoublePending = false;
};
