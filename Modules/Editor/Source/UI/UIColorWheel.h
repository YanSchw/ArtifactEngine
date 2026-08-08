#pragma once
#include "GameFramework/UINode.h"
#include <functional>
#include "UIColorWheel.gen.h"

class UIColorWheel : public UINode {
public:
    ARTIFACT_CLASS();

    UIColorWheel();

    float Hue = 0.0f;
    float Saturation = 0.0f;
    float Value = 1.0f;
    std::function<void()> Changed;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;

private:
    void PickAt(const Vec2& InCursorPos);
    Vec2 GetCenter() const { return m_Geometry.Min() + m_Geometry.Size * 0.5f; }
    float GetRadius() const { return glm::min(m_Geometry.Size.x, m_Geometry.Size.y) * 0.5f - 1.0f; }
};
