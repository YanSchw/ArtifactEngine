#pragma once
#include "GameFramework/UINode.h"
#include "DetailsItem.gen.h"

class DetailsTab;

/** Base of everything stacked in a Details panel. Parents drive visibility and height through
 *  MeasureHeight so collapsing and search filtering stay consistent across nesting levels. */
class DetailsItem : public UINode {
public:
    ARTIFACT_CLASS();

    DetailsTab* OwnerTab = nullptr;
    int32_t Depth = 0;

    virtual float MeasureHeight(const String& InFilter) const { (void)InFilter; return 0.0f; }
};
