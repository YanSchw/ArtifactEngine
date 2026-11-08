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
#include <algorithm>

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
    const float padding = Depth == 0 ? BodyPadding : 0.0f;
    return HeaderHeight + ((open && bodyHeight > 0.0f) ? bodyHeight + padding : 0.0f);
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
        item->Depth = std::max(item->Depth, item->As<DetailsCategory>() ? Depth + 1 : Depth);
        const float height = open ? item->MeasureHeight(effective) : 0.0f;
        item->SetEnabled(height > 0.0f);
        item->Size = { 1.0_rel, height };
        bodyHeight += height;
    }
    m_Body->SetEnabled(open && bodyHeight > 0.0f);
    m_Body->Size = { 1.0_rel, bodyHeight };

    const float indent = Depth == 0 ? 4.0f : (float)Depth * IndentWidth - 6.0f;
    m_Arrow->Position = Vec2(indent, (HeaderHeight - 12.0f) * 0.5f);
    m_TitleLabel->Position = Vec2(indent + 14.0f, 0.0f);
    m_TitleLabel->Color = Depth == 0 ? EditorStyle::TextBright : EditorStyle::Text;
    if (Depth == 0) {
        m_TitleLabel->Size = { 1.0_rel - UIValue::Px(indent + 20.0f), HeaderHeight };
    } else {
        const float split = OwnerTab ? OwnerTab->GetLabelSplit() : 0.32f;
        m_TitleLabel->Size = { UIValue(split, -(indent + 22.0f)), HeaderHeight };
    }
    m_Arrow->Image = AssetManager::Get().GetAsset<VectorImage>(UUID::FromString(
        open ? "b1c2d3e4-0002-4a00-9000-000000000002" : "b1c2d3e4-0003-4a00-9000-000000000003"));
}

void DetailsCategory::Paint(UIDrawList& OutDrawList) {
    const UIRectF header(m_Geometry.Position, Vec2(m_Geometry.Size.x, HeaderHeight));
    const UIRectF bottom(Vec2(header.Min().x, header.Max().y - 1.0f), Vec2(header.Size.x, 1.0f));
    if (Depth == 0) {
        OutDrawList.AddRect(header, IsHovered() ? EditorStyle::TabHover : EditorStyle::Button, m_WorldMatrix);
        OutDrawList.AddRect(bottom, EditorStyle::Border, m_WorldMatrix);
        return;
    }

    OutDrawList.AddRect(header, Vec4(1.0f, 1.0f, 1.0f, IsHovered() ? 0.06f : 0.03f), m_WorldMatrix);
    const float split = OwnerTab ? OwnerTab->GetLabelSplit() : 0.32f;
    const UIRectF splitter(Vec2(header.Min().x + header.Size.x * split, header.Min().y), Vec2(1.0f, HeaderHeight));
    OutDrawList.AddRect(splitter, EditorStyle::Border, m_WorldMatrix);
    OutDrawList.AddRect(bottom, EditorStyle::Border, m_WorldMatrix);
    PaintIndentGutter(OutDrawList, Depth - 1, header);
}

void DetailsCategory::OnPressed(const Vec2& InCursorPos) {
    const UIRectF header(m_Geometry.Position, Vec2(m_Geometry.Size.x, HeaderHeight));
    if (HitTestRect(header, InCursorPos)) {
        m_Expanded = !m_Expanded;
    }
}
