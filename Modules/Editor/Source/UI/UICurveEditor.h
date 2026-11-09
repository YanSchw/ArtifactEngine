#pragma once
#include "GameFramework/UINode.h"
#include <functional>
#include "UICurveEditor.gen.h"

struct ParticleCurve;
struct ParticleCurveKey;

class UICurveEditor : public UINode {
public:
    ARTIFACT_CLASS();

    UICurveEditor();

    std::function<ParticleCurve*()> GetCurve;
    std::function<void()> BeginChange;
    std::function<void()> Changed;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
    virtual bool OnSecondaryClick(const Vec2& InCursorPos) override;

private:
    UIRectF GetPlot() const;
    Vec2 ToScreen(float InTime, float InValue) const;
    ParticleCurveKey KeyAt(const Vec2& InCursorPos) const;
    int32_t FindKey(const Vec2& InCursorPos) const;
    void UpdateRange();
    void Edit(const std::function<void(ParticleCurve&)>& InEdit);

    Vec2 m_Range = Vec2(0.0f, 1.0f);
    int32_t m_DraggedKey = -1;
};
