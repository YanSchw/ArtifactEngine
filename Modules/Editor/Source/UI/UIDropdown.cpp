#include "UIDropdown.h"
#include "EditorStyle.h"
#include "EditorIcons.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIImage.h"
#include "UIRoundedQuad.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIScrollArea.h"
#include "Rendering/Texture.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>
#include <cctype>

static constexpr float s_ItemHeight = 22.0f;
static constexpr float s_ThumbnailItemHeight = 30.0f;
static constexpr float s_SearchHeight = 30.0f;
static constexpr float s_MinPopupWidth = 100.0f;
static constexpr float s_MinThumbnailPopupWidth = 190.0f;
static constexpr float s_MaxPopupHeight = 300.0f;
static constexpr float s_IconSize = 14.0f;
static constexpr float s_TextInset = 8.0f;
static constexpr float s_IconTextInset = 27.0f;

static String ToLower(const String& InText) {
    String lowered = InText;
    for (char& character : lowered) {
        character = (char)std::tolower((unsigned char)character);
    }
    return lowered;
}

UIDropdown::UIDropdown() {
    Size = { 1.0_rel, 20.0f };
    Interactable = true;
    Cursor = CursorIcon::Hand;

    m_ValueIcon = Add<UISvg>();
    m_ValueIcon->Anchor = m_ValueIcon->Pivot = Vec2(0.0f, 0.5f);
    m_ValueIcon->Position = Vec2(7.0f, 0.0f);
    m_ValueIcon->Size = Vec2(s_IconSize);
    m_ValueIcon->Tint = EditorStyle::Text;
    m_ValueIcon->SetEnabled(false);

    m_ValueLabel = Add<UILabel>();
    m_ValueLabel->Anchor = m_ValueLabel->Pivot = Vec2(0.0f, 0.5f);
    m_ValueLabel->Position = Vec2(s_TextInset, 0.0f);
    m_ValueLabel->Size = { 1.0_rel - 26.0_px, 1.0_rel };
    m_ValueLabel->FontSize = EditorStyle::FontSize;
    m_ValueLabel->Color = EditorStyle::Text;
    m_ValueLabel->VAlign = UIVAlign::Middle;

    m_Arrow = Add<UISvg>();
    m_Arrow->Anchor = m_Arrow->Pivot = Vec2(1.0f, 0.5f);
    m_Arrow->Position = Vec2(-5.0f, 0.0f);
    m_Arrow->Size = Vec2(10.0f, 10.0f);
    m_Arrow->Tint = EditorStyle::TextDim;
    m_Arrow->Image = EditorIcons::ArrowDown();
}

UIDropdown::~UIDropdown() {
    if (UIDropdownPopup* popup = m_Popup.Get()) {
        delete popup;
    }
}

void UIDropdown::OnBind() {
    UINode::OnBind();
    if (UIDropdownPopup* popup = m_Popup.Get()) {
        if (popup->IsCloseRequested()) {
            delete popup;
            m_Popup = nullptr;
        }
    }
    if (GetSelectedLabel) {
        m_ValueLabel->Text = GetSelectedLabel();
    }

    VectorImage* icon = GetSelectedIcon ? GetSelectedIcon() : nullptr;
    m_ValueIcon->Image = icon;
    m_ValueIcon->SetEnabled(icon != nullptr);
    const float labelLeft = icon ? s_IconTextInset : s_TextInset;
    m_ValueLabel->Position = Vec2(labelLeft, 0.0f);
    m_ValueLabel->Size = { 1.0_rel - UIValue::Px(labelLeft + 20.0f), 1.0_rel };

    const bool open = IsOpen();
    m_Arrow->Image = open ? EditorIcons::ArrowUp() : EditorIcons::ArrowDown();
    m_Arrow->Tint = (open || IsHovered()) ? EditorStyle::Text : EditorStyle::TextDim;
}

void UIDropdown::Paint(UIDrawList& OutDrawList) {
    const Vec4 border = IsOpen() ? EditorStyle::Accent : EditorStyle::ButtonBorder;
    const Vec4 fill = IsHovered() || IsOpen() ? EditorStyle::Button : EditorStyle::PanelDark;
    const UIRectF inner(m_Geometry.Min() + Vec2(1.0f), m_Geometry.Size - Vec2(2.0f));
    OutDrawList.AddRoundedRect(m_Geometry, border, EditorStyle::ButtonRadius, m_WorldMatrix);
    OutDrawList.AddRoundedRect(inner, fill, EditorStyle::ButtonRadius - 1.0f, m_WorldMatrix);
}

void UIDropdown::OnClick() {
    if (m_Popup.Get()) {
        ClosePopup();
        return;
    }
    UICanvas* canvas = GetCanvas();
    if (!canvas) {
        return;
    }
    UIDropdownPopup* popup = canvas->Add<UIDropdownPopup>();
    popup->Build(this, GetOptions ? GetOptions() : Array<UIDropdownOption>(), GetSelectedIndex ? GetSelectedIndex() : -1);
    m_Popup = popup;
}

bool UIDropdown::IsOpen() const {
    UIDropdownPopup* popup = m_Popup.Get();
    return popup && !popup->IsCloseRequested();
}

void UIDropdown::ClosePopup() {
    if (UIDropdownPopup* popup = m_Popup.Get()) {
        popup->RequestClose();
    }
}

UIDropdownPopup::UIDropdownPopup() {
    Fill();
    Interactable = true;
}

float UIDropdownPopup::RowHeight() const {
    return m_HasThumbnails ? s_ThumbnailItemHeight : s_ItemHeight;
}

void UIDropdownPopup::Build(UIDropdown* InOwner, const Array<UIDropdownOption>& InOptions, int32_t InSelected) {
    m_Owner = InOwner;
    m_Options = InOptions;
    m_Selected = InSelected;
    for (const UIDropdownOption& option : m_Options) {
        m_HasThumbnails = m_HasThumbnails || option.Thumbnail;
        m_HasIcons = m_HasIcons || option.Icon;
    }

    UIRoundedQuad* border = Add<UIRoundedQuad>();
    border->Anchor = border->Pivot = Vec2(0.0f);
    border->Color = EditorStyle::ButtonBorder;
    border->CornerRadius = Vec4(EditorStyle::ButtonRadius);
    border->Padding = UIPadding(1.0f);
    border->Interactable = true;
    m_Panel = border;

    UIRoundedQuad* background = border->Add<UIRoundedQuad>();
    background->Fill();
    background->Color = EditorStyle::PanelDark;
    background->CornerRadius = Vec4(EditorStyle::ButtonRadius - 1.0f);
    background->Padding = UIPadding(3.0f);

    UIVStack* column = background->Add<UIVStack>();
    column->Fill();

    if (!InOwner->SearchPlaceholder.empty()) {
        UINode* searchRow = column->Add<UINode>();
        searchRow->Size = { 1.0_rel, UIValue(s_SearchHeight) };

        UISvg* icon = searchRow->Add<UISvg>();
        icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
        icon->Position = Vec2(11.0f, 0.0f);
        icon->Size = Vec2(11.0f, 11.0f);
        icon->Image = EditorIcons::Search();
        icon->Tint = EditorStyle::TextDim;

        m_Search = searchRow->Add<UITextArea>();
        m_Search->Anchor = m_Search->Pivot = Vec2(0.0f, 0.5f);
        m_Search->Position = Vec2(26.0f, 0.0f);
        m_Search->Size = { 1.0_rel - 32.0_px, 22.0_px };
        m_Search->SingleLine = true;
        m_Search->Placeholder = InOwner->SearchPlaceholder;
        m_Search->FontSize = EditorStyle::FontSize;
        m_Search->TextColor = EditorStyle::Text;
        m_Search->PlaceholderColor = EditorStyle::TextDim;
        m_Search->BackgroundColor = Vec4(0.0f);
        m_Search->FocusedBorderColor = Vec4(0.0f);
        m_Search->RequestFocus();
    }

    UIScrollArea* scroll = column->Add<UIScrollArea>();
    scroll->Size = { 1.0_rel, 1.0_rel };

    UIVStack* list = scroll->Add<UIVStack>();
    list->Anchor = list->Pivot = Vec2(0.0f);
    list->Position = Vec2(0.0f);
    list->Size = { 1.0_rel, 0.0_px };
    m_List = list;

    RebuildRows();
}

void UIDropdownPopup::RebuildRows() {
    while (m_List->HasChildren()) {
        delete m_List->GetChild(0);
    }

    m_ListHeight = 0.0f;
    for (int32_t i = 0; i < m_Options.Size(); i++) {
        if (m_Filter.empty() || ToLower(m_Options[i].Label).find(m_Filter) != String::npos) {
            AddRow(i);
            m_ListHeight += RowHeight();
        }
    }
}

float UIDropdownPopup::LabelLeft() const {
    return m_HasThumbnails ? RowHeight() : (m_HasIcons ? s_IconTextInset : s_TextInset);
}

void UIDropdownPopup::AddRow(int32_t InIndex) {
    const UIDropdownOption& option = m_Options[InIndex];
    const float height = RowHeight();
    const float labelLeft = LabelLeft();
    const bool selected = InIndex == m_Selected;

    UIButton* item = m_List->Add<UIButton>();
    item->Size = { 1.0_rel, UIValue(height) };
    item->NormalColor = selected ? Vec4(Vec3(EditorStyle::Accent), 0.35f) : Vec4(0.0f);
    item->HoverColor = selected ? Vec4(Vec3(EditorStyle::Accent), 0.5f) : EditorStyle::TabHover;
    item->PressedColor = EditorStyle::ButtonPressed;
    item->CornerRadius = Vec4(EditorStyle::ButtonRadius - 1.0f);
    item->Clicked = [this, InIndex] {
        UIDropdown* owner = m_Owner.Get();
        RequestClose();
        if (!owner) {
            return;
        }
        owner->ClosePopup();
        // The callback may tear this popup down, so nothing here may touch it afterwards.
        if (owner->SelectionChanged) {
            owner->SelectionChanged(InIndex);
        }
    };

    if (m_HasThumbnails) {
        UISvg* icon = item->Add<UISvg>();
        icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
        icon->Position = Vec2(5.0f, 0.0f);
        icon->Size = Vec2(height - 10.0f);
        icon->Image = option.Icon;
        icon->Tint = option.IconTint;

        UIImage* preview = item->Add<UIImage>();
        preview->Anchor = preview->Pivot = Vec2(0.0f, 0.5f);
        preview->Position = Vec2(3.0f, 0.0f);
        preview->Size = Vec2(height - 6.0f);
        preview->Image = option.Thumbnail;

        Texture* thumbnail = option.Thumbnail;
        item->Bind = [preview, icon, thumbnail] {
            const bool ready = thumbnail && thumbnail->GetDefaultView().Get();
            preview->SetEnabled(ready);
            icon->SetEnabled(!ready);
        };
    } else if (option.Icon) {
        UISvg* icon = item->Add<UISvg>();
        icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
        icon->Position = Vec2(7.0f, 0.0f);
        icon->Size = Vec2(s_IconSize);
        icon->Image = option.Icon;
        icon->Tint = option.IconTint;
    }

    UILabel* label = item->Add<UILabel>();
    label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
    label->Position = Vec2(labelLeft, 0.0f);
    label->Size = { 1.0_rel - UIValue::Px(labelLeft + 8.0f), 1.0_rel };
    label->FontSize = EditorStyle::FontSize;
    label->Color = selected ? EditorStyle::TextBright : EditorStyle::Text;
    label->VAlign = UIVAlign::Middle;
    label->Text = option.Label;
}

void UIDropdownPopup::OnBind() {
    UINode::OnBind();
    UIDropdown* owner = m_Owner.Get();
    if (!owner) {
        RequestClose();
        return;
    }

    if (m_Search) {
        const String filter = ToLower(m_Search->Text);
        if (filter != m_Filter) {
            m_Filter = filter;
            RebuildRows();
        }
    }

    const UIRectF field = owner->GetGeometry();
    const float chrome = 8.0f + (m_Search ? s_SearchHeight : 0.0f);
    const float minWidth = m_HasThumbnails ? s_MinThumbnailPopupWidth : s_MinPopupWidth;
    float height = std::min(m_ListHeight + chrome, s_MaxPopupHeight);

    const float width = std::max(field.Size.x, minWidth);
    float x = field.Min().x;
    float y = field.Max().y + 2.0f;
    if (UICanvas* canvas = GetCanvas()) {
        const UIRectF rect = canvas->GetGeometry();
        x = std::max(rect.Min().x + 2.0f, std::min(x, rect.Max().x - 2.0f - width));
        height = std::min(height, rect.Size.y - 4.0f);
        // Flipping above the field beats squeezing the list against the bottom edge.
        if (y + height > rect.Max().y - 2.0f && field.Min().y - height - 2.0f > rect.Min().y) {
            y = field.Min().y - height - 2.0f;
        }
        y = std::min(y, rect.Max().y - 2.0f - height);
    }

    m_Panel->Position = Vec2(x, std::max(0.0f, y));
    m_Panel->Size = { width, height };
}

void UIDropdownPopup::OnPressed(const Vec2& InCursorPos) {
    (void)InCursorPos;
    if (UIDropdown* owner = m_Owner.Get()) {
        owner->ClosePopup();
    }
    RequestClose();
}
