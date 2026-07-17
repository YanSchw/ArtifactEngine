#pragma once
#include "GameFramework/UIToggle.h"
#include "UICheckbox.gen.h"

/** An editor-styled checkbox. */
class UICheckbox : public UIToggle {
public:
    ARTIFACT_CLASS();

    UICheckbox();

    virtual void Paint(UIDrawList& OutDrawList) override;
};
