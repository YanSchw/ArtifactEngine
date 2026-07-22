#pragma once
#include "GameFramework/UINode.h"
#include "UIGrid.gen.h"

/** Lays children out in fixed-size cells, left-to-right then top-to-bottom, wrapping to a new row
 *  when the content width runs out. */
class UIGrid : public UINode {
public:
    ARTIFACT_CLASS();

    Vec2 CellSize = Vec2(96.0f, 112.0f);
    Vec2 Gap = Vec2(12.0f, 14.0f);

protected:
    virtual void LayoutChildren(const UIRectF& InContent) override;
};
