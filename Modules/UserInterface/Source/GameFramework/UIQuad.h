#pragma once
#include "UINode.h"
#include "UIQuad.gen.h"

/** A solid colored rectangle. */
class UIQuad : public UINode {
public:
    ARTIFACT_CLASS();

    /* RGBA 0..1; painted only when alpha > 0. */
    Vec4 Color = Vec4(1.0f);

    virtual void Paint(UIDrawList& OutDrawList) override;
};
