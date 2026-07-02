#pragma once
#include "UINode.h"
#include "UIForEach.gen.h"

/** A data-driven list. Each frame it keeps its child count equal to Count(); new items are built
 *  once via BuildItem (so per-item state persists), and give each item a Bind lambda for live
 *  values. Defaults to a vertical (SplitY) stack. Prefer the UI::ForEach() builder. */
class UIForEach : public UINode {
public:
    ARTIFACT_CLASS();

    UIForEach();

    std::function<int()> Count;
    std::function<void(UINode& item, int index)> BuildItem;

    virtual void OnBind() override;
};
