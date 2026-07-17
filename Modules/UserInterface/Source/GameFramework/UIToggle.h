#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include <functional>
#include "UIToggle.gen.h"

/** A logic-only toggle that paints nothing of its own. */
class UIToggle : public UINode {
public:
    ARTIFACT_CLASS();

    UIToggle();

    bool IsOn = false;
    WeakObjectPtr<UINode> Graphic;
    std::function<void(bool)> Changed;

    virtual void OnClick() override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
};
