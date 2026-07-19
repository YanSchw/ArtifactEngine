#include "DetailsTab.h"
#include "MajorTab.h"
#include "Details/DetailsCustomization.h"
#include "Details/DetailsItem.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIScrollArea.h"
#include <algorithm>
#include <cctype>

static String ToLower(const String& InText) {
    String lower = InText;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return lower;
}

DetailsTab::DetailsTab() {
    UIVStack* layout = Add<UIVStack>();
    layout->Fill();

    m_HeaderHost = layout->Add<UINode>();
    m_HeaderHost->Size = { 1.0_rel, 0.0_px };

    m_SearchBar = layout->Add<UIQuad>();
    m_SearchBar->Size = { 1.0_rel, 30.0f };
    m_SearchBar->Color = EditorStyle::TabBar;

    m_SearchField = m_SearchBar->Add<UITextArea>();
    m_SearchField->Center({ 1.0_rel - 12.0_px, 22.0f });
    m_SearchField->SingleLine = true;
    m_SearchField->Placeholder = "Search...";
    m_SearchField->FontSize = EditorStyle::FontSize;
    m_SearchField->TextColor = EditorStyle::Text;
    m_SearchField->PlaceholderColor = EditorStyle::TextDim;
    m_SearchField->BackgroundColor = EditorStyle::PanelDark;
    m_SearchField->FocusedBorderColor = EditorStyle::Accent;
    m_SearchField->Padding = UIPadding(6.0f, 0.0f);

    m_Scroll = layout->Add<UIScrollArea>();
    m_Scroll->Size = { 1.0_rel, 1.0_rel };

    m_List = m_Scroll->Add<UIVStack>();
    m_List->Anchor = m_List->Pivot = Vec2(0.0f);
    m_List->Position = Vec2(0.0f);
    m_List->Size = { 1.0_rel, 0.0_px };

    m_MessageLabel = Add<UILabel>();
    m_MessageLabel->Fill();
    m_MessageLabel->FontSize = EditorStyle::FontSize;
    m_MessageLabel->Color = EditorStyle::TextDim;
    m_MessageLabel->HAlign = UIHAlign::Center;
    m_MessageLabel->VAlign = UIVAlign::Middle;

    Bind = [this] { Refresh(); };
}

void DetailsTab::SetLabelSplit(float InSplit) {
    m_LabelSplit = std::clamp(InSplit, 0.2f, 0.8f);
}

bool DetailsTab::MatchesFilter(const String& InText, const String& InLowerFilter) {
    return InLowerFilter.empty() || ToLower(InText).find(InLowerFilter) != String::npos;
}

void DetailsTab::Refresh() {
    m_Filter = ToLower(m_SearchField->Text);

    MajorTab* major = GetMajorTab();
    const Array<Object*> selection = major ? major->GetSelection() : Array<Object*>();
    Object* sole = selection.Size() == 1 ? selection[0] : nullptr;
    DetailsCustomization* customization = sole ? DetailsCustomization::FindFor(sole->GetClass()) : nullptr;
    Object* inspected = customization ? sole : nullptr;

    if (inspected != m_Inspected.Get()) {
        RebuildInspector(inspected, customization);
        m_Scroll->ScrollOffset = Vec2(0.0f);
    }

    const bool inspecting = m_Inspected.Get() != nullptr;
    m_HeaderHost->SetEnabled(inspecting);
    m_HeaderHost->Size = { 1.0_rel, inspecting ? m_HeaderHeight : 0.0f };
    m_SearchBar->SetEnabled(inspecting);
    m_SearchBar->Size = { 1.0_rel, inspecting ? 30.0f : 0.0f };
    m_Scroll->SetEnabled(inspecting);

    m_MessageLabel->SetEnabled(!inspecting);
    if (!inspecting) {
        if (selection.IsEmpty()) {
            m_MessageLabel->Text = "Nothing selected.";
        } else if (selection.Size() > 1) {
            m_MessageLabel->Text = std::to_string(selection.Size()) + " objects selected. Multi-editing is not supported yet.";
        } else {
            m_MessageLabel->Text = selection[0]->GetClass().Name + " cannot be edited here yet.";
        }
        return;
    }

    float total = 0.0f;
    for (uint32_t i = 0; i < m_List->GetChildCount(); i++) {
        DetailsItem* item = m_List->GetChild((int)i)->As<DetailsItem>();
        if (!item) {
            continue;
        }
        const float height = item->MeasureHeight(m_Filter);
        item->SetEnabled(height > 0.0f);
        item->Size = { 1.0_rel, height };
        total += height;
    }
    m_List->Size = { 1.0_rel, total };
}

void DetailsTab::RebuildInspector(Object* InObject, DetailsCustomization* InCustomization) {
    while (m_HeaderHost->GetChildCount() > 0) {
        delete m_HeaderHost->GetChild((int)m_HeaderHost->GetChildCount() - 1);
    }
    while (m_List->GetChildCount() > 0) {
        delete m_List->GetChild((int)m_List->GetChildCount() - 1);
    }
    m_Inspected = InObject;
    m_HeaderHeight = 0.0f;
    if (InObject && InCustomization) {
        m_HeaderHeight = InCustomization->BuildHeader(*m_HeaderHost, InObject, *this);
        InCustomization->BuildContent(*m_List, InObject, *this);
    }
}
