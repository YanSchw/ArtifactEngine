#pragma once
#include "MinorTab.h"
#include "DetailsTab.gen.h"

/** Edits the properties of the selected node. Placeholder content until selection exists. */
class DetailsTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    DetailsTab();

    virtual String GetTabTitle() const override { return "Details"; }
};
