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

bool UIButton::OnSecondaryClick(const Vec2& InCursorPos) {
    if (!SecondaryClicked) {
        return false;
    }
    SecondaryClicked(InCursorPos);
    return true;
}

void UIButton::Paint(UIDrawList& OutDrawList) {
    const Vec4 color = (IsPressed() && IsHovered()) ? PressedColor : (IsHovered() ? HoverColor : NormalColor);
    if (CornerRadius == Vec4(0.0f) && BorderColor.a <= 0.0f) {
        OutDrawList.AddRect(m_Geometry, color, m_WorldMatrix);
        return;
    }

    UIRectF fill = m_Geometry;
    Vec4 radius = CornerRadius;
    if (BorderColor.a > 0.0f) {
        OutDrawList.AddRoundedRectEx(m_Geometry, BorderColor, BorderColor,
                                     radius.x, radius.y, radius.z, radius.w, m_WorldMatrix);
        fill = UIRectF(m_Geometry.Min() + Vec2(1.0f), m_Geometry.Size - Vec2(2.0f));
        radius = glm::max(radius - Vec4(1.0f), Vec4(0.0f));
    }
    OutDrawList.AddRoundedRectEx(fill, color, color, radius.x, radius.y, radius.z, radius.w, m_WorldMatrix);
}
