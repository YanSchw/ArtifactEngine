#pragma once
#include "GameFramework/UINode.h"
#include <functional>
#include "UIGradientEditor.gen.h"

struct ParticleGradient;

class UIGradientEditor : public UINode {
public:
    ARTIFACT_CLASS();

    UIGradientEditor();

    std::function<ParticleGradient*()> GetGradient;
    std::function<void()> BeginChange;
    std::function<void()> Changed;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
    virtual bool OnSecondaryClick(const Vec2& InCursorPos) override;

private:
    UIRectF GetBar() const;
    float TimeAt(const Vec2& InCursorPos) const;
    int32_t FindKey(const Vec2& InCursorPos) const;
    void Edit(const std::function<void(ParticleGradient&)>& InEdit);

    int32_t m_DraggedKey = -1;
};
