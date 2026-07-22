#pragma once
#include "HeroTool.h"
#include <cstdint>
#include "ConsoleTool.gen.h"

class UIStack;
class UIScrollArea;

class ConsoleTool : public HeroTool {
public:
    ARTIFACT_CLASS();

    virtual String GetTitle() const override { return "Console"; }
    virtual bool IsRightAligned() const override { return true; }
    virtual float GetStatusButtonWidth() const override { return 118.0f; }
    virtual void BuildDrawer(UINode& InBody) override;
    virtual void Tick(float InDeltaTime) override;
    virtual bool BuildStatusWidget(UINode& InButton) override;

private:
    void RebuildLog();

    uint64_t m_SeenVersion = 0;
    UIStack* m_List = nullptr;
    UIScrollArea* m_Scroll = nullptr;
};
