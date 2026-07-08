#pragma once
#include "UINode.h"
#include "UIStack.gen.h"

enum class UIAxis : uint8_t { X = 0, Y = 1 };

/** Arranges children in a row (X) or column (Y), separated by Gap. */
class UIStack : public UINode {
public:
    ARTIFACT_CLASS();

    UIAxis Axis = UIAxis::Y;
    UIValue Gap = 0.0f;

protected:
    virtual void LayoutChildren(const UIRectF& InContent) override;
};
