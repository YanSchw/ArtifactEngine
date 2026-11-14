#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include "Common/Delegate.h"
#include <functional>
#include "UIToggle.gen.h"

/** A logic-only toggle that paints nothing of its own. */
class UIToggle : public UINode {
public:
    ARTIFACT_CLASS();

    UIToggle();

    PROPERTY()
    bool IsOn = false;
    WeakObjectPtr<UINode> Graphic;
    std::function<void(bool)> Changed;

    PROPERTY(Params = (IsOn))
    Delegate<bool> OnChanged;

    FUNCTION(Pure)
    bool IsToggledOn() const { return IsOn; }
    FUNCTION()
    void SetToggledOn(bool InIsOn);

    virtual void OnClick() override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
};
