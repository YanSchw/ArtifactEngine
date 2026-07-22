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

bool ConsoleTool::BuildStatusWidget(UINode& InButton) {
    const auto placeIcon = [&InButton](float InX, float InSize, VectorImage* InImage, const Vec4& InTint) {
        UISvg* svg = InButton.Add<UISvg>();
        svg->Anchor = svg->Pivot = Vec2(0.0f, 0.5f);
        svg->Position = Vec2(InX, 0.0f);
        svg->Size = Vec2(InSize, InSize);
        svg->Image = InImage;
        svg->Tint = InTint;
    };
    const auto placeCount = [&InButton](float InX, const Vec4& InColor, std::function<int()> InCount) {
        UILabel* label = InButton.Add<UILabel>();
        label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
        label->Position = Vec2(InX, 0.0f);
        label->Size = { 22.0_px, 1.0_rel };
        label->FontSize = EditorStyle::FontSize - 1.0f;
        label->VAlign = UIVAlign::Middle;
        label->Color = InColor;
        label->Bind = [label, count = std::move(InCount)] { label->Text = std::to_string(count()); };
    };

    placeIcon(31.0f, 13.0f, EditorIcons::Warning(), s_WarnColor);
    placeCount(46.0f, s_WarnColor, [] { return Logging::GetWarningCount(); });
    placeIcon(70.0f, 13.0f, EditorIcons::Error(), s_ErrorColor);
    placeCount(85.0f, s_ErrorColor, [] { return Logging::GetErrorCount(); });
    return true;
}
