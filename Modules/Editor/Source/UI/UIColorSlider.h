#pragma once
#include "GameFramework/UINode.h"
#include <functional>
#include "UIColorSlider.gen.h"

class UIColorSlider : public UINode {
public:
    ARTIFACT_CLASS();

    UIColorSlider();

    float Value = 1.0f;
    Color TopColor = Color(1.0f);
    Color BottomColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    bool Translucent = false;
    std::function<void()> Changed;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;

private:
    void PickAt(const Vec2& InCursorPos);
};
