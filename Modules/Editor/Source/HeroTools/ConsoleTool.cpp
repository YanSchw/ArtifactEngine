#include "ConsoleTool.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "GameFramework/UINode.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UIScrollArea.h"
#include "Core/Log.h"
#include <string>

static const Vec4 s_WarnColor = HexColor(0xE0A63A);
static const Vec4 s_ErrorColor = HexColor(0xE0574B);

static void ClearChildren(UINode* InNode) {
    while (InNode->HasChildren()) {
        delete InNode->GetChild(0);
    }
}

static Vec4 ColorForLevel(LogLevel InLevel) {
    switch (InLevel) {
        case LogLevel::WARN: return s_WarnColor;
        case LogLevel::ERROR:
        case LogLevel::CRITICAL: return s_ErrorColor;
        case LogLevel::TRACE:
        case LogLevel::DEBUG: return EditorStyle::TextDim;
        default: return EditorStyle::Text;
    }
}

void ConsoleTool::BuildDrawer(UINode& InBody) {
    m_List = nullptr;
    m_Scroll = nullptr;
    m_SeenVersion = 0;

    UIVStack* root = InBody.Add<UIVStack>();
    root->Fill();

    UIQuad* header = root->Add<UIQuad>();
    header->Size = { 1.0_rel, 30.0_px };
    header->Color = EditorStyle::ToolBar;

    UIHStack* headerRow = header->Add<UIHStack>();
    headerRow->Fill();
    headerRow->Padding = UIPadding(10.0f, 4.0f);
    headerRow->Gap = 8.0f;

    UILabel* title = headerRow->Add<UILabel>();
    title->Size = { 1.0_rel, 1.0_rel };
    title->FontSize = EditorStyle::FontSize;
    title->Color = EditorStyle::TextDim;
    title->VAlign = UIVAlign::Middle;
    title->Text = "Output Log";

    UIButton* clear = headerRow->Add<UIButton>();
    clear->Size = { 70.0_px, 1.0_rel };
    clear->SetCaption("Clear");
    EditorStyle::ApplyButtonStyle(*clear);
    clear->Clicked = [] { Logging::Clear(); };

    m_Scroll = root->Add<UIScrollArea>();
    m_Scroll->Size = { 1.0_rel, 1.0_rel };

    UIVStack* list = m_Scroll->Add<UIVStack>();
    list->Fill();
    list->Padding = UIPadding(10.0f, 6.0f);
    list->Gap = 1.0f;
    m_List = list;
}

void ConsoleTool::Tick(float InDeltaTime) {
    (void)InDeltaTime;
    const uint64_t version = Logging::GetLogVersion();
    if (version != m_SeenVersion) {
        m_SeenVersion = version;
        RebuildLog();
    }
}

void ConsoleTool::RebuildLog() {
    if (!m_List) {
        return;
    }
    ClearChildren(m_List);

    std::vector<LogEntry> entries = Logging::GetRecentEntries();
    for (const LogEntry& entry : entries) {
        UILabel* row = m_List->Add<UILabel>();
        row->Size = { 1.0_rel, 16.0_px };
        row->FontSize = EditorStyle::FontSize - 1.0f;
        row->HAlign = UIHAlign::Left;
        row->VAlign = UIVAlign::Middle;
        row->Color = ColorForLevel(entry.Level);
        row->Text = "[" + entry.Category + "] " + entry.Message;
    }

    // Keep the newest lines in view.
    if (m_Scroll) {
        m_Scroll->ScrollOffset.y = 1.0e6f;
    }
}

static constexpr float s_TallyInset = 8.0f;
static constexpr float s_TallyColumnWidth = 36.0f;
static constexpr float s_TallyIconSize = 13.0f;
static constexpr float s_TallyIconTop = 2.0f;
static constexpr float s_TallyCountTop = s_TallyIconTop + s_TallyIconSize + 1.0f;
static constexpr float s_TallyCountHeight = 11.0f;

float ConsoleTool::GetLogStatusWidth() {
    return s_TallyInset * 2.0f + s_TallyColumnWidth * 3.0f;
}

void ConsoleTool::BuildLogStatus(UINode& InHost) {
    struct Tally {
        VectorImage* Icon;
        Vec4 Color;
        int (*Count)();
    };
    const Tally tallies[] = {
        { EditorIcons::Message(), EditorStyle::Text, &Logging::GetMessageCount },
        { EditorIcons::Warning(), s_WarnColor,       &Logging::GetWarningCount },
        { EditorIcons::Error(),   s_ErrorColor,      &Logging::GetErrorCount },
    };

    float x = s_TallyInset;
    for (const Tally& tally : tallies) {
        UISvg* icon = InHost.Add<UISvg>();
        icon->Anchor = icon->Pivot = Vec2(0.0f, 0.0f);
        icon->Position = Vec2(x + (s_TallyColumnWidth - s_TallyIconSize) * 0.5f, s_TallyIconTop);
        icon->Size = Vec2(s_TallyIconSize, s_TallyIconSize);
        icon->Image = tally.Icon;
        icon->Tint = tally.Color;

        UILabel* label = InHost.Add<UILabel>();
        label->Anchor = label->Pivot = Vec2(0.0f, 0.0f);
        label->Position = Vec2(x, s_TallyCountTop);
        label->Size = { UIValue(s_TallyColumnWidth), UIValue(s_TallyCountHeight) };
        label->FontSize = EditorStyle::FontSize - 3.0f;
        label->HAlign = UIHAlign::Center;
        label->VAlign = UIVAlign::Middle;
        label->Color = tally.Color;

        label->Bind = [label, icon, color = tally.Color, count = tally.Count] {
            const int value = count();
            label->Text = std::to_string(value);
            const Vec4 tint(color.r, color.g, color.b, value > 0 ? color.a : color.a * 0.4f);
            label->Color = tint;
            icon->Tint = tint;
        };

        x += s_TallyColumnWidth;
    }
}

