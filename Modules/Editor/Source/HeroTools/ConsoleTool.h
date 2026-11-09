#pragma once
#include "HeroTool.h"
#include "Core/Log.h"
#include <cstdint>
#include "ConsoleTool.gen.h"

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
    enum class Severity : uint8_t { Message = 0, Warning = 1, Error = 2 };

    struct Line {
        LogEntry Entry;
        int32_t Repeats = 1;
    };

    static Severity SeverityOf(LogLevel InLevel);

    void BuildToolbar(UINode& InToolbar);
    void AddSeverityChip(UINode& InToolbar, Severity InSeverity);
    void RefilterLines();
    void RebuildVisibleRows();
    void AddRow(const Line& InLine, int32_t InIndex);

    std::vector<LogEntry> m_Entries;
    std::vector<Line> m_Lines;
    Array<String> m_Categories;
    int32_t m_SeverityCounts[3] = { 0, 0, 0 };
    bool m_ShowSeverity[3] = { true, true, true };
    String m_Search;
    String m_Category;
    bool m_Collapse = false;

    uint64_t m_SeenVersion = ~0ull;
    bool m_FilterDirty = true;
    int32_t m_FirstRow = -1;
    int32_t m_RowCount = -1;
    UINode* m_List = nullptr;
    UIScrollArea* m_Scroll = nullptr;
    UINode* m_Empty = nullptr;
};
