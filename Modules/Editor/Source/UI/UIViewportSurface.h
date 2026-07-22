#pragma once
#include "GameFramework/UIImage.h"
#include <functional>
#include "UIViewportSurface.gen.h"

/** The rendered scene image, reporting pointer events in render-target pixels so the viewport can
 *  hit-test against what the renderer drew. */
class UIViewportSurface : public UIImage {
public:
    ARTIFACT_CLASS();

    UIViewportSurface();

    std::function<void(const Vec2&)> Pressed;
    std::function<void(const Vec2&, const Vec2&)> Dragged;
    std::function<void(bool)> Released;

    Vec2 ToRenderPixel(const Vec2& InScreenPos) const;

    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
};
