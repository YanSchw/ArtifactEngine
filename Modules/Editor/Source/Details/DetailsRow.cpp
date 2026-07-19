#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UISvg.h"
#include "Assets/AssetManager.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

DetailsRow::DetailsRow() {
    Size = { 1.0_rel, RowHeight };
    Interactable = true;

    m_NameLabel = Add<UILabel>();
    m_NameLabel->Anchor = m_NameLabel->Pivot = Vec2(0.0f, 0.5f);
    m_NameLabel->FontSize = EditorStyle::FontSize;
    m_NameLabel->Color = EditorStyle::Text;
    m_NameLabel->VAlign = UIVAlign::Middle;

    m_ValueHost = Add<UINode>();
    m_ValueHost->Anchor = m_ValueHost->Pivot = Vec2(0.0f, 0.5f);

    m_ResetButton = Add<UIButton>();
    m_ResetButton->Anchor = m_ResetButton->Pivot = Vec2(1.0f, 0.5f);
    m_ResetButton->Position = Vec2(-3.0f, 0.0f);
    m_ResetButton->Size = Vec2(20.0f, 20.0f);
    m_ResetButton->NormalColor = Vec4(0.0f);
    m_ResetButton->HoverColor = EditorStyle::TabHover;
    m_ResetButton->PressedColor = EditorStyle::ButtonPressed;
    m_ResetButton->Clicked = [this] {
        if (ResetAction) {
            ResetAction();
        }
    };

    UISvg* resetIcon = m_ResetButton->Add<UISvg>();
    resetIcon->Center(Vec2(13.0f, 13.0f));
    resetIcon->Tint = EditorStyle::TextDim;
    resetIcon->Image = AssetManager::Get().GetAsset<VectorImage>(UUID::FromString("b1c2d3e4-0006-4a00-9000-000000000006"));
}

void DetailsRow::SetLabel(const String& InLabel) {
    m_Label = InLabel;
    m_NameLabel->Text = InLabel;
}

float DetailsRow::MeasureHeight(const String& InFilter) const {
    return DetailsTab::MatchesFilter(m_Label, InFilter) ? RowHeight : 0.0f;
}

void DetailsRow::OnBind() {
    UINode::OnBind();
    const float split = OwnerTab ? OwnerTab->GetLabelSplit() : 0.32f;
    const float indent = 8.0f + (float)Depth * 12.0f;

    m_NameLabel->Position = Vec2(indent, 0.0f);
    m_NameLabel->Size = { UIValue(split, -(indent + 8.0f)), RowHeight };

    m_ValueHost->Position = { UIValue(split, 5.0f), 0.0f };
    m_ValueHost->Size = { UIValue(1.0f - split, -(5.0f + 3.0f + ResetColumnWidth)), RowHeight - 4.0f };
}

void DetailsRow::Paint(UIDrawList& OutDrawList) {
    if (IsHovered()) {
        OutDrawList.AddRect(m_Geometry, Vec4(1.0f, 1.0f, 1.0f, 0.03f), m_WorldMatrix);
    }
    const float split = OwnerTab ? OwnerTab->GetLabelSplit() : 0.32f;
    const float splitX = m_Geometry.Min().x + m_Geometry.Size.x * split;
    const UIRectF splitter(Vec2(splitX, m_Geometry.Min().y), Vec2(1.0f, m_Geometry.Size.y));
    OutDrawList.AddRect(splitter, m_DraggingSplit ? EditorStyle::Accent : EditorStyle::Border, m_WorldMatrix);

    const UIRectF bottom(Vec2(m_Geometry.Min().x, m_Geometry.Max().y - 1.0f), Vec2(m_Geometry.Size.x, 1.0f));
    OutDrawList.AddRect(bottom, EditorStyle::Border, m_WorldMatrix);
}

bool DetailsRow::IsNearSplitter(const Vec2& InScreenPos) const {
    const float split = OwnerTab ? OwnerTab->GetLabelSplit() : 0.32f;
    const float splitX = m_Geometry.Min().x + m_Geometry.Size.x * split;
    const UIRectF grab(Vec2(splitX - 4.0f, m_Geometry.Min().y), Vec2(8.0f, m_Geometry.Size.y));
    return HitTestRect(grab, InScreenPos);
}

void DetailsRow::OnPressed(const Vec2& InCursorPos) {
    m_DraggingSplit = IsNearSplitter(InCursorPos);
}

void DetailsRow::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (!m_DraggingSplit || !OwnerTab || m_Geometry.Size.x <= 0.0f) {
        return;
    }
    OwnerTab->SetLabelSplit((InCursorPos.x - m_Geometry.Min().x) / m_Geometry.Size.x);
}

void DetailsRow::OnReleased(bool InInside) {
    (void)InInside;
    m_DraggingSplit = false;
}

void DetailsRow::OnUIUpdate(const UIFrameContext& InContext) {
    Cursor = (m_DraggingSplit || IsNearSplitter(InContext.CursorPosition)) ? CursorIcon::ResizeH : CursorIcon::Arrow;
}
