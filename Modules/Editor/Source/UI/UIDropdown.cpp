#include "UIDropdown.h"
#include "EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UICanvas.h"
#include "Assets/AssetManager.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

static constexpr float s_ItemHeight = 22.0f;

UIDropdown::UIDropdown() {
    Size = { 1.0_rel, 20.0f };
    Interactable = true;
    Cursor = CursorIcon::Hand;

    m_ValueLabel = Add<UILabel>();
    m_ValueLabel->Anchor = m_ValueLabel->Pivot = Vec2(0.0f, 0.5f);
    m_ValueLabel->Position = Vec2(6.0f, 0.0f);
    m_ValueLabel->Size = { 1.0_rel - 26.0_px, 1.0_rel };
    m_ValueLabel->FontSize = EditorStyle::FontSize;
    m_ValueLabel->Color = EditorStyle::Text;
    m_ValueLabel->VAlign = UIVAlign::Middle;

    m_Arrow = Add<UISvg>();
    m_Arrow->Anchor = m_Arrow->Pivot = Vec2(1.0f, 0.5f);
    m_Arrow->Position = Vec2(-5.0f, 0.0f);
    m_Arrow->Size = Vec2(10.0f, 10.0f);
    m_Arrow->Tint = EditorStyle::TextDim;
    m_Arrow->Image = AssetManager::Get().GetAsset<VectorImage>(UUID::FromString("b1c2d3e4-0002-4a00-9000-000000000002"));
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
}

void UIDropdown::Paint(UIDrawList& OutDrawList) {
    OutDrawList.AddRoundedRect(m_Geometry, IsHovered() ? EditorStyle::Button : EditorStyle::PanelDark, 2.0f, m_WorldMatrix);
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
    popup->Build(this, GetOptions ? GetOptions() : Array<String>(), GetSelectedIndex ? GetSelectedIndex() : -1);
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

void UIDropdownPopup::Build(UIDropdown* InOwner, const Array<String>& InOptions, int32_t InSelected) {
    m_Owner = InOwner;

    UIQuad* border = Add<UIQuad>();
    border->Color = EditorStyle::Border;
    border->Padding = UIPadding(1.0f);
    m_Panel = border;

    UIQuad* background = border->Add<UIQuad>();
    background->Fill();
    background->Color = EditorStyle::PanelDark;

    UIVStack* list = background->Add<UIVStack>();
    list->Fill();

    for (int32_t i = 0; i < InOptions.Size(); i++) {
        UIButton& item = UI::Button(*list, InOptions[i], [this, i] {
            if (UIDropdown* owner = m_Owner.Get()) {
                if (owner->SelectionChanged) {
                    owner->SelectionChanged(i);
                }
                owner->ClosePopup();
            }
            RequestClose();
        });
        item.Size = { 1.0_rel, s_ItemHeight };
        item.NormalColor = (i == InSelected) ? EditorStyle::Accent : Vec4(0.0f);
        item.HoverColor = EditorStyle::TabHover;
        item.PressedColor = EditorStyle::ButtonPressed;
        if (Node* child = item.GetChildByClass(UILabel::StaticClass())) {
            UILabel* label = child->As<UILabel>();
            label->FontSize = EditorStyle::FontSize;
            label->Color = EditorStyle::Text;
            label->HAlign = UIHAlign::Left;
            label->Fill();
            label->Position = Vec2(8.0f, 0.0f);
            label->VAlign = UIVAlign::Middle;
        }
    }

    m_Panel->Anchor = m_Panel->Pivot = Vec2(0.0f);
    m_Panel->Size = Vec2(100.0f, (float)InOptions.Size() * s_ItemHeight + 2.0f);
}

void UIDropdownPopup::OnBind() {
    UINode::OnBind();
    UIDropdown* owner = m_Owner.Get();
    if (!owner) {
        RequestClose();
        return;
    }
    const UIRectF field = owner->GetGeometry();
    const float height = m_Panel->Size.Y.Pixels;
    float y = field.Max().y + 2.0f;
    if (UICanvas* canvas = GetCanvas()) {
        y = std::min(y, canvas->GetGeometry().Max().y - height);
    }
    m_Panel->Position = Vec2(field.Min().x, std::max(0.0f, y));
    m_Panel->Size = { std::max(field.Size.x, 100.0f), height };
}

void UIDropdownPopup::OnPressed(const Vec2& InCursorPos) {
    (void)InCursorPos;
    if (UIDropdown* owner = m_Owner.Get()) {
        owner->ClosePopup();
    }
    RequestClose();
}
