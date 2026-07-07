#pragma once
#include "UIStack.h"
#include "UIVStack.gen.h"

/** A UIStack fixed to the vertical axis: children flow top to bottom. */
class UIVStack : public UIStack {
public:
    ARTIFACT_CLASS();

    UIVStack() { Axis = UIAxis::Y; }
};
