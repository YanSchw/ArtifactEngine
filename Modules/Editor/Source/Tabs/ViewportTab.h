#pragma once
#include "MinorTab.h"
#include "ViewportTab.gen.h"

class ViewportTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    virtual String GetTabTitle() const override { return "Viewport"; }
    virtual bool HasTransparentBackground() const override { return true; }
};
