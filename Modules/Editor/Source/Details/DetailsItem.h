#pragma once
#include "GameFramework/UINode.h"
#include "DetailsItem.gen.h"

class DetailsTab;
class UIDrawList;

/** Base of everything stacked in a Details panel. Parents drive visibility and height through
 *  MeasureHeight so collapsing and search filtering stay consistent across nesting levels. */
class DetailsItem : public UINode {
public:
    ARTIFACT_CLASS();

    static constexpr float IndentWidth = 12.0f;

    DetailsTab* OwnerTab = nullptr;
    int32_t Depth = 0;
    /** Set by the enclosing category while its toggle is off; the item is drawn dimmed. */
    bool Inactive = false;

    virtual float MeasureHeight(const String& InFilter) const { (void)InFilter; return 0.0f; }

protected:
    /** One darker band per nesting level along the left of InRect, so depth reads down the column. */
    void PaintIndentGutter(UIDrawList& OutDrawList, int32_t InLevels, const UIRectF& InRect) const;
};