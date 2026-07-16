#include "UIButton.h"
#include "UILabel.h"
#include "Rendering/UIDrawList.h"

UIButton::UIButton() {
    Size = Vec2(160.0f, 40.0f);
    Interactable = true;
    Cursor = CursorIcon::Hand;
}

void UIButton::SetCaption(const String& InText) {
    if (!m_Label) {
        UILabel* label = Add<UILabel>();
        label->Fill();
        label->HAlign = UIHAlign::Center;
        label->VAlign = UIVAlign::Middle;
        m_Label = label;
    }
    m_Label->Text = InText;
}

void UIButton::Paint(UIDrawList& OutDrawList) {
    const Vec4 color = (IsPressed() && IsHovered()) ? PressedColor : (IsHovered() ? HoverColor : NormalColor);
    OutDrawList.AddRect(m_Geometry, color, m_WorldMatrix);
}
