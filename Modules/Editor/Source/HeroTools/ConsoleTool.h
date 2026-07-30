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
    virtual bool HasStatusButton() const override { return false; }
    virtual void BuildDrawer(UINode& InBody) override;
    virtual void Tick(float InDeltaTime) override;
    void BuildLogStatus(UINode& InHost);
    static float GetLogStatusWidth();

private:
    void RebuildLog();

    uint64_t m_SeenVersion = 0;
    UIStack* m_List = nullptr;
    UIScrollArea* m_Scroll = nullptr;
};
