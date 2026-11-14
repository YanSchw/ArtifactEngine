#pragma once
#include "DetailsItem.h"
#include "Object/Pointer.h"
#include "AnimationTransitionPreview.gen.h"

class AnimationGraph;
class AnimationTransition;

/** Both states on one time axis: drag the blend to move the exit time, its right edge to change
 *  the duration, the next state's bar to change the offset. */
class AnimationTransitionPreview : public DetailsItem {
public:
    ARTIFACT_CLASS();

    static constexpr float Height = 84.0f;

    AnimationTransitionPreview();

    WeakObjectPtr<AnimationTransition> Transition;
    WeakObjectPtr<AnimationGraph> Graph;

    virtual float MeasureHeight(const String& InFilter) const override;
    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;

private:
    enum class Handle : uint8_t { None, ExitTime, Duration, Offset };

    struct Layout {
        String SourceName;
        String TargetName;
        float SourceLength = 1.0f;
        float TargetLength = 1.0f;
        float ExitAt = 0.0f;
        float BlendEnd = 0.0f;
        float TargetStart = 0.0f;
        float ViewStart = 0.0f;
        float ViewEnd = 1.0f;
        UIRectF Track;
        UIRectF SourceBar;
        UIRectF TargetBar;

        float ToX(float InSeconds) const;
        float SecondsPerPixel() const;
    };

    bool ComputeLayout(Layout& OutLayout) const;
    Handle HandleAt(const Layout& InLayout, const Vec2& InCursorPos) const;
    void PaintBar(UIDrawList& OutDrawList, const Layout& InLayout, const UIRectF& InRow, float InStart, float InLength,
                  float InActiveFrom, float InActiveTo, const String& InName);
    void PaintRuler(UIDrawList& OutDrawList, const Layout& InLayout);

    Handle m_Dragged = Handle::None;
    float m_PressX = 0.0f;
    float m_PressValue = 0.0f;
    float m_SecondsPerPixel = 0.0f;
    float m_SourceLength = 1.0f;
    float m_TargetLength = 1.0f;
};
