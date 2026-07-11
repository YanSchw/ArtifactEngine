#pragma once
#include "GameFramework/UINode.h"
#include <functional>
#include "UICaptionButton.gen.h"

enum class UICaptionAction : uint8_t { Minimize = 0, Maximize, Close };

/** A title bar window-control button (minimize / maximize / close) with a custom-drawn icon,
 *  used on platforms where the editor draws its own window chrome. */
class UICaptionButton : public UINode {
public:
    ARTIFACT_CLASS();

    UICaptionButton();

    UICaptionAction Action = UICaptionAction::Minimize;
    std::function<void()> Clicked;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnClick() override { if (Clicked) Clicked(); }
};
