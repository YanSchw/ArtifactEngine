#pragma once
#include "UINode.h"
#include "Object/Enum.h"
#include "UIStack.gen.h"

ARTIFACT_ENUM();
enum class UIAxis : uint8_t { X = 0, Y = 1 };

/** Arranges children in a row (X) or column (Y), separated by Gap. */
class UIStack : public UINode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    UIAxis Axis = UIAxis::Y;
    PROPERTY()
    UIValue Gap = 0.0f;

protected:
    virtual void LayoutChildren(const UIRectF& InContent) override;
};
