#pragma once
#include "UINode.h"
#include "UIIf.gen.h"

/** Conditional content. Builds its subtree once (Build), then shows/hides it each frame based on
 *  Condition() — so toggling preserves the content's state. Prefer the UI::If() builder. */
class UIIf : public UINode {
public:
    ARTIFACT_CLASS();

    std::function<bool()> Condition;
    std::function<void(UINode& content)> Build;

    virtual void OnBind() override;

private:
    UINode* m_Content = nullptr;
};
