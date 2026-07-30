#pragma once
#include "GameFramework/UINode.h"
#include "UIRoundedQuad.gen.h"

/** A solid colored rectangle with per-corner rounding. */
class UIRoundedQuad : public UINode {
public:
    ARTIFACT_CLASS();

    /* RGBA 0..1; painted only when alpha > 0. */
    Vec4 Color = Vec4(1.0f);
    /* Corner radius in pixels, ordered top-left, top-right, bottom-right, bottom-left. */
    Vec4 CornerRadius = Vec4(0.0f);

    virtual void Paint(UIDrawList& OutDrawList) override;
};
