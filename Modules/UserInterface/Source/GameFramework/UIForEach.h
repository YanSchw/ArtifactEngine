#pragma once
#include "UIStack.h"
#include "UIForEach.gen.h"

/** A data-driven list. Each frame it keeps its child count equal to Count(); new items are built
 *  once via BuildItem (so per-item state persists), and give each item a Bind lambda for live
 *  values. It is a UIStack (vertical by default; item roots Fill(), so give an item a fixed
 *  main-axis Size in BuildItem for packed rows). Prefer the UI::ForEach() builder. */
class UIForEach : public UIStack {
public:
    ARTIFACT_CLASS();

    std::function<int()> Count;
    std::function<void(UINode& item, int index)> BuildItem;

    virtual void OnBind() override;
};
