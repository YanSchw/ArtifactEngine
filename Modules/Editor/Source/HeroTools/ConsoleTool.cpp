#include "ConsoleTool.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIDropdown.h"
#include "Assets/Font.h"
#include "GameFramework/UINode.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UITextArea.h"
#include "UI/UIRoundedQuad.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <format>
#include <map>
#include <string>

static const Vec4 s_WarnColor = HexColor(0xE0A63A);
static const Vec4 s_ErrorColor = HexColor(0xE0574B);
static constexpr float s_ToolbarHeight = 36.0f;
static constexpr float s_RowHeight = 20.0f;
static constexpr float s_RowIconX = 8.0f;
static constexpr float s_TimeX = 26.0f;
static constexpr float s_TimeWidth = 64.0f;
static constexpr float s_CategoryX = s_TimeX + s_TimeWidth;
static constexpr float s_CategoryWidth = 150.0f;
static constexpr float s_MessageX = s_CategoryX + s_CategoryWidth;
static constexpr float s_BadgeWidth = 44.0f;
static constexpr int32_t s_RowMargin = 4;

static String Lowered(const String& InText) {
    String lowered = InText;
    for (char& character : lowered) {
        character = (char)std::tolower((unsigned char)character);
    }
    return lowered;
}

static String FormatTime(int64_t InTimeMs) {
    const std::time_t seconds = (std::time_t)(InTimeMs / 1000);
    std::tm local = {};
#if defined(_WIN32)
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif
    return std::format("{:02}:{:02}:{:02}", local.tm_hour, local.tm_min, local.tm_sec);
}

static Vec4 SeverityColor(int32_t InSeverity) {
    return InSeverity == 2 ? s_ErrorColor : InSeverity == 1 ? s_WarnColor : EditorStyle::Text;
}

static VectorImage* SeverityIcon(int32_t InSeverity) {
    return InSeverity == 2 ? EditorIcons::Error() : InSeverity == 1 ? EditorIcons::Warning() : EditorIcons::Message();
}

ConsoleTool::Severity ConsoleTool::SeverityOf(LogLevel InLevel) {
    switch (InLevel) {
        case LogLevel::WARN: return Severity::Warning;
        case LogLevel::ERROR:
        case LogLevel::CRITICAL: return Severity::Error;
        default: return Severity::Message;
    }
}

void ConsoleTool::BuildDrawer(UINode& InBody) {
    m_List = nullptr;
    m_Scroll = nullptr;
    m_Empty = nullptr;
    m_SeenVersion = ~0ull;
    m_FirstRow = -1;
    m_RowCount = -1;

    UIVStack* root = InBody.Add<UIVStack>();
    root->Fill();

    UIQuad* toolbar = root->Add<UIQuad>();
    toolbar->Size = { 1.0_rel, UIValue(s_ToolbarHeight) };
    toolbar->Color = EditorStyle::ToolBar;
    BuildToolbar(*toolbar);

    UIQuad* divider = root->Add<UIQuad>();
    divider->Size = { 1.0_rel, 1.0_px };
    divider->Color = EditorStyle::Border;

    UIQuad* body = root->Add<UIQuad>();
    body->Size = { 1.0_rel, 1.0_rel };
    body->Color = EditorStyle::PanelDark;

    m_Scroll = body->Add<UIScrollArea>();
    m_Scroll->Fill();
    m_Scroll->StickToBottom = true;
    m_List = m_Scroll->Add<UINode>();
    m_List->Anchor = m_List->Pivot = Vec2(0.0f);
    m_List->Size = { 1.0_rel, 0.0_px };

    UILabel* empty = body->Add<UILabel>();
    empty->Fill();
    empty->HAlign = UIHAlign::Center;
    empty->VAlign = UIVAlign::Middle;
    empty->FontSize = EditorStyle::FontSize;
    empty->Color = EditorStyle::TextDim;
    empty->Bind = [this, empty] {
        empty->Text = !m_Lines.empty() ? String()
                    : m_Entries.empty() ? String("The log is empty")
                    : String("No messages match the current filters");
    };
    m_Empty = empty;
}

void ConsoleTool::BuildToolbar(UINode& InToolbar) {
    UIHStack* row = InToolbar.Add<UIHStack>();
    row->Fill();
    row->Padding = UIPadding(8.0f, 5.0f);
    row->Gap = 6.0f;

    UINode* search = row->Add<UINode>();
    search->Size = { 240.0_px, 1.0_rel };
    UITextArea* searchField = search->Add<UITextArea>();
    searchField->Fill();
    searchField->Padding = UIPadding(24.0f, 0.0f, 6.0f, 0.0f);
    searchField->SingleLine = true;
    searchField->Placeholder = "Search log...";
    searchField->FontSize = EditorStyle::FontSize;
    searchField->TextColor = EditorStyle::Text;
    searchField->PlaceholderColor = EditorStyle::TextDim;
    searchField->BackgroundColor = EditorStyle::PanelDark;
    searchField->FocusedBorderColor = EditorStyle::Accent;
    searchField->CornerRadius = EditorStyle::ButtonRadius;
    searchField->Text = m_Search;
    searchField->TextChanged = [this](const String& InText) {
        m_Search = InText;
        m_FilterDirty = true;
    };
    UISvg* searchIcon = search->Add<UISvg>();
    searchIcon->Anchor = searchIcon->Pivot = Vec2(0.0f, 0.5f);
    searchIcon->Position = Vec2(8.0f, 0.0f);
    searchIcon->Size = Vec2(11.0f);
    searchIcon->Image = EditorIcons::Search();
    searchIcon->Tint = EditorStyle::TextDim;

    AddSeverityChip(*row, Severity::Message);
    AddSeverityChip(*row, Severity::Warning);
    AddSeverityChip(*row, Severity::Error);

    UIDropdown* category = row->Add<UIDropdown>();
    category->Size = { 190.0_px, 1.0_rel };
    category->SearchPlaceholder = "Filter categories";
    category->GetOptions = [this] {
        Array<UIDropdownOption> options = { UIDropdownOption("All categories") };
        for (const String& name : m_Categories) {
            options.Add(UIDropdownOption(name));
        }
        return options;
    };
    category->GetSelectedIndex = [this] { return m_Category.empty() ? 0 : m_Categories.IndexOf(m_Category) + 1; };
    category->GetSelectedLabel = [this] { return m_Category.empty() ? String("All categories") : m_Category; };
    category->SelectionChanged = [this](int32_t InIndex) {
        m_Category = (InIndex <= 0 || InIndex > m_Categories.Size()) ? String() : m_Categories[InIndex - 1];
        m_FilterDirty = true;
    };
    category->Add<UITooltip>()->Text = "Category";

    UIButton& collapse = UI::Button(*row, "Collapse", [this] {
        m_Collapse = !m_Collapse;
        m_FilterDirty = true;
    });
    collapse.Size = { 78.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(collapse);
    collapse.Add<UITooltip>()->Text = "Merge repeated lines into one";
    UIButton* collapsePtr = &collapse;
    collapse.Bind = [this, collapsePtr] { EditorStyle::SetToggled(*collapsePtr, m_Collapse); };

    row->Add<UINode>()->Size = { 1.0_rel, 1.0_rel };

    UIButton& clear = UI::Button(*row, "Clear", [] { Logging::Clear(); });
    clear.Size = { 64.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(clear);
}

void ConsoleTool::AddSeverityChip(UINode& InToolbar, Severity InSeverity) {
    static const char* s_Names[3] = { "Messages", "Warnings", "Errors" };
    const int32_t index = (int32_t)InSeverity;
    const Vec4 color = SeverityColor(index);

    UIButton* chip = InToolbar.Add<UIButton>();
    chip->Size = { 64.0_px, 1.0_rel };
    EditorStyle::StyleButton(*chip);
    chip->Clicked = [this, index] {
        m_ShowSeverity[index] = !m_ShowSeverity[index];
        m_FilterDirty = true;
    };
    chip->Add<UITooltip>()->Text = std::string("Show ") + s_Names[index];

    UISvg* icon = chip->Add<UISvg>();
    icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
    icon->Position = Vec2(9.0f, 0.0f);
    icon->Size = Vec2(13.0f);
    icon->Image = SeverityIcon(index);

    UILabel* count = chip->Add<UILabel>();
    count->Anchor = count->Pivot = Vec2(0.0f, 0.5f);
    count->Position = Vec2(27.0f, 0.0f);
    count->Size = { 1.0_rel - 30.0_px, 1.0_rel };
    count->FontSize = EditorStyle::FontSize;
    count->VAlign = UIVAlign::Middle;

    chip->Bind = [this, chip, icon, count, index, color] {
        const bool shown = m_ShowSeverity[index];
        const Vec4 tint = shown ? color : EditorStyle::TextDim;
        chip->NormalColor = shown ? Vec4(Vec3(color), 0.14f) : EditorStyle::Button;
        chip->HoverColor = shown ? Vec4(Vec3(color), 0.22f) : EditorStyle::ButtonHover;
        chip->BorderColor = shown ? Vec4(Vec3(color), 0.45f) : EditorStyle::ButtonBorder;
        icon->Tint = tint;
        count->Color = tint;
        count->Text = std::to_string(m_SeverityCounts[index]);
    };
}

void ConsoleTool::Tick(float InDeltaTime) {
    (void)InDeltaTime;
    if (!m_List || !m_Scroll) {
        return;
    }

    const uint64_t version = Logging::GetLogVersion();
    if (version != m_SeenVersion) {
        m_SeenVersion = version;
        m_Entries = Logging::GetRecentEntries();
        m_FilterDirty = true;
    }
    if (m_FilterDirty) {
        m_FilterDirty = false;
        RefilterLines();
        m_RowCount = -1;
    }

    m_List->Size = { 1.0_rel, UIValue((float)m_Lines.size() * s_RowHeight) };
    RebuildVisibleRows();
}

void ConsoleTool::RefilterLines() {
    m_Lines.clear();
    m_SeverityCounts[0] = m_SeverityCounts[1] = m_SeverityCounts[2] = 0;

    std::map<String, int32_t> categories;
    const String search = Lowered(m_Search);
    for (const LogEntry& entry : m_Entries) {
        const int32_t severity = (int32_t)SeverityOf(entry.Level);
        categories[entry.Category]++;
        if (!m_Category.empty() && entry.Category != m_Category) {
            continue;
        }
        if (!search.empty() && Lowered(entry.Message).find(search) == String::npos
            && Lowered(entry.Category).find(search) == String::npos) {
            continue;
        }
        m_SeverityCounts[severity]++;
        if (!m_ShowSeverity[severity]) {
            continue;
        }
        if (m_Collapse && !m_Lines.empty()) {
            Line& last = m_Lines.back();
            if (last.Entry.Level == entry.Level && last.Entry.Category == entry.Category && last.Entry.Message == entry.Message) {
                last.Repeats++;
                last.Entry.TimeMs = entry.TimeMs;
                continue;
            }
        }
        m_Lines.push_back({ entry, 1 });
    }

    m_Categories.Clear();
    for (const auto& [name, count] : categories) {
        m_Categories.Add(name);
    }
}

void ConsoleTool::RebuildVisibleRows() {
    const float viewHeight = m_Scroll->GetGeometry().Size.y;
    const int32_t first = std::max(0, (int32_t)(m_Scroll->ScrollOffset.y / s_RowHeight) - s_RowMargin);
    const int32_t count = std::min((int32_t)m_Lines.size() - first, (int32_t)(viewHeight / s_RowHeight) + 2 * s_RowMargin + 1);
    if (first == m_FirstRow && count == m_RowCount) {
        return;
    }
    m_FirstRow = first;
    m_RowCount = count;

    while (m_List->HasChildren()) {
        delete m_List->GetChild(0);
    }
    for (int32_t i = 0; i < count; i++) {
        AddRow(m_Lines[first + i], first + i);
    }
}

void ConsoleTool::AddRow(const Line& InLine, int32_t InIndex) {
    const int32_t severity = (int32_t)SeverityOf(InLine.Entry.Level);
    const Vec4 color = SeverityColor(severity);
    const bool dim = InLine.Entry.Level == LogLevel::TRACE || InLine.Entry.Level == LogLevel::DEBUG;

    UIQuad* row = m_List->Add<UIQuad>();
    row->Anchor = row->Pivot = Vec2(0.0f);
    row->Position = Vec2(0.0f, (float)InIndex * s_RowHeight);
    row->Size = { 1.0_rel, UIValue(s_RowHeight) };
    row->Color = severity == 2 ? Vec4(Vec3(s_ErrorColor), 0.10f)
               : severity == 1 ? Vec4(Vec3(s_WarnColor), 0.07f)
               : (InIndex % 2 == 1 ? Vec4(1.0f, 1.0f, 1.0f, 0.02f) : Vec4(0.0f));

    UISvg* icon = row->Add<UISvg>();
    icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
    icon->Position = Vec2(s_RowIconX, 0.0f);
    icon->Size = Vec2(12.0f);
    icon->Image = SeverityIcon(severity);
    icon->Tint = severity == 0 ? Vec4(Vec3(EditorStyle::TextDim), 0.6f) : color;

    const auto addLabel = [row](float InX, UIValue InWidth, const String& InText, const Vec4& InColor) {
        UILabel* label = row->Add<UILabel>();
        label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
        label->Position = Vec2(InX, 0.0f);
        label->Size = { InWidth, 1.0_rel };
        label->FontSize = EditorStyle::FontSize - 1.0f;
        label->VAlign = UIVAlign::Middle;
        label->Color = InColor;
        label->Text = InText;
        return label;
    };
    addLabel(s_TimeX, UIValue(s_TimeWidth), FormatTime(InLine.Entry.TimeMs), EditorStyle::TextDim);
    addLabel(s_CategoryX, UIValue(s_CategoryWidth - 8.0f), InLine.Entry.Category, Vec4(Vec3(EditorStyle::TextDim), 0.85f));
    const float badge = InLine.Repeats > 1 ? s_BadgeWidth + 8.0f : 8.0f;
    addLabel(s_MessageX, 1.0_rel - UIValue::Px(s_MessageX + badge), InLine.Entry.Message,
             dim ? EditorStyle::TextDim : (severity == 0 ? EditorStyle::Text : color));

    if (InLine.Repeats > 1) {
        UIRoundedQuad* pill = row->Add<UIRoundedQuad>();
        pill->Anchor = pill->Pivot = Vec2(1.0f, 0.5f);
        pill->Position = Vec2(-10.0f, 0.0f);
        pill->Size = Vec2(s_BadgeWidth, 15.0f);
        pill->Color = EditorStyle::Button;
        pill->CornerRadius = Vec4(7.5f);
        UILabel* repeats = pill->Add<UILabel>();
        repeats->Fill();
        repeats->HAlign = UIHAlign::Center;
        repeats->VAlign = UIVAlign::Middle;
        repeats->FontSize = EditorStyle::FontSize - 2.0f;
        repeats->Color = EditorStyle::Text;
        repeats->Text = std::to_string(InLine.Repeats);
    }
}


static constexpr float s_TallyInset = 8.0f;
static constexpr float s_TallyColumnWidth = 46.0f;
static constexpr float s_TallyIconSize = 13.0f;
static constexpr float s_TallyIconGap = 4.0f;

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
        icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
        icon->Position = Vec2(x, 0.0f);
        icon->Size = Vec2(s_TallyIconSize, s_TallyIconSize);
        icon->Image = tally.Icon;
        icon->Tint = tally.Color;

        UILabel* label = InHost.Add<UILabel>();
        label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
        label->Position = Vec2(x + s_TallyIconSize + s_TallyIconGap, 0.0f);
        label->Size = { UIValue(s_TallyColumnWidth - s_TallyIconSize - s_TallyIconGap), 1.0_rel };
        label->FontSize = EditorStyle::FontSize - 1.0f;
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
