#pragma once
#include "GameFramework/UINode.h"
#include "UIDockSplitter.gen.h"

/** The draggable divider between the two children of a split UIDockNode. */
class UIDockSplitter : public UINode {
public:
    ARTIFACT_CLASS();

    UIDockSplitter();

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
};
