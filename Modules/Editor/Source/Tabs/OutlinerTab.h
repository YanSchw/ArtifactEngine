#pragma once
#include "MinorTab.h"
#include "OutlinerTab.gen.h"

/** Lists the nodes of the edited scene. Placeholder rows until scenes are editable. */
class OutlinerTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    OutlinerTab();

    virtual String GetTabTitle() const override { return "Outliner"; }
};
