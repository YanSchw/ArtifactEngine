#pragma once
#include "UIStack.h"
#include "UIHStack.gen.h"

/** A UIStack fixed to the horizontal axis: children flow left to right. */
class UIHStack : public UIStack {
public:
    ARTIFACT_CLASS();

    UIHStack() { Axis = UIAxis::X; }
};
