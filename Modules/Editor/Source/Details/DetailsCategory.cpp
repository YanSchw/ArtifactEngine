#include "DetailsCategory.h"
#include "Tabs/DetailsTab.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIVStack.h"
#include "Assets/AssetManager.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"
#include "Rendering/UIDrawList.h"

DetailsCategory::DetailsCategory() {
    Interactable = true;

    m_Arrow = Add<UISvg>();
    m_Arrow->Anchor = m_Arrow->Pivot = Vec2(0.0f);
    m_Arrow->Size = Vec2(12.0f, 12.0f);
    m_Arrow->Tint = EditorStyle::TextDim;

    m_TitleLabel = Add<UILabel>();
    m_TitleLabel->Anchor = m_TitleLabel->Pivot = Vec2(0.0f);
    m_TitleLabel->Size = { 1.0_rel - 24.0_px, HeaderHeight };
    m_TitleLabel->FontSize = EditorStyle::FontSize;
    m_TitleLabel->Color = EditorStyle::TextBright;
    m_TitleLabel->VAlign = UIVAlign::Middle;

    m_Body = Add<UIVStack>();
    m_Body->Anchor = m_Body->Pivot = Vec2(0.0f);
    m_Body->Position = Vec2(0.0f, HeaderHeight);
    m_Body->Size = { 1.0_rel, 0.0_px };
}

void DetailsCategory::SetTitle(const String& InTitle) {
    m_Title = InTitle;
    m_TitleLabel->Text = InTitle;
}

UINode* DetailsCategory::GetBody() const {
    return m_Body;
}

bool DetailsCategory::TitleMatches(const String& InFilter) const {
    return DetailsTab::MatchesFilter(m_Title, InFilter);
}

float DetailsCategory::MeasureHeight(const String& InFilter) const {
    const String effective = (InFilter.empty() || TitleMatches(InFilter)) ? String() : InFilter;
    float bodyHeight = 0.0f;
    for (uint32_t i = 0; i < m_Body->GetChildCount(); i++) {
        if (DetailsItem* item = m_Body->GetChild((int)i)->As<DetailsItem>()) {
            bodyHeight += item->MeasureHeight(effective);
        }
    }
    if (!InFilter.empty() && !effective.empty() && bodyHeight <= 0.0f) {
        return 0.0f;
    }
    const bool open = m_Expanded || !InFilter.empty();
    return HeaderHeight + ((open && bodyHeight > 0.0f) ? bodyHeight + BodyPadding : 0.0f);
}

void DetailsCategory::OnBind() {
    UINode::OnBind();

    const String filter = OwnerTab ? OwnerTab->GetFilter() : String();
    const String effective = (filter.empty() || TitleMatches(filter)) ? String() : filter;
    const bool open = m_Expanded || !filter.empty();

    float bodyHeight = 0.0f;
    for (uint32_t i = 0; i < m_Body->GetChildCount(); i++) {
        DetailsItem* item = m_Body->GetChild((int)i)->As<DetailsItem>();
        if (!item) {
            continue;
        }
        const float height = open ? item->MeasureHeight(effective) : 0.0f;
        item->SetEnabled(height > 0.0f);
        item->Size = { 1.0_rel, height };
        bodyHeight += height;
    }
    m_Body->SetEnabled(open && bodyHeight > 0.0f);
    m_Body->Size = { 1.0_rel, bodyHeight };

    const float indent = 4.0f + (float)Depth * 12.0f;
    m_Arrow->Position = Vec2(indent, (HeaderHeight - 12.0f) * 0.5f);
    m_TitleLabel->Position = Vec2(indent + 16.0f, 0.0f);
    m_Arrow->Image = AssetManager::Get().GetAsset<VectorImage>(UUID::FromString(
        open ? "b1c2d3e4-0002-4a00-9000-000000000002" : "b1c2d3e4-0003-4a00-9000-000000000003"));
}

void DetailsCategory::Paint(UIDrawList& OutDrawList) {
    const UIRectF header(m_Geometry.Position, Vec2(m_Geometry.Size.x, HeaderHeight));
    OutDrawList.AddRect(header, IsHovered() ? EditorStyle::TabHover : EditorStyle::Button, m_WorldMatrix);
}

void DetailsCategory::OnPressed(const Vec2& InCursorPos) {
    const UIRectF header(m_Geometry.Position, Vec2(m_Geometry.Size.x, HeaderHeight));
    if (HitTestRect(header, InCursorPos)) {
        m_Expanded = !m_Expanded;
    }
}
