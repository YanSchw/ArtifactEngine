#include "UIButton.h"
#include "UILabel.h"
#include "Rendering/UIDrawList.h"

UIButton::UIButton() {
    SizeDelta = Vec2(160.0f, 40.0f);
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

void UIButton::OnUIUpdate(const UIFrameContext& InContext) {
    m_Hovered = HitTest(InContext.MousePosition);

    if (m_Hovered && InContext.MousePressedThisFrame) {
        m_Pressed = true;
    }
    if (InContext.MouseReleasedThisFrame) {
        if (m_Pressed && m_Hovered && OnClick) {
            OnClick();
        }
        m_Pressed = false;
    }
    if (!InContext.MouseDown) {
        m_Pressed = false;
    }
}

void UIButton::Paint(UIDrawList& OutDrawList) {
    const Vec4 color = (m_Pressed && m_Hovered) ? PressedColor : (m_Hovered ? HoverColor : NormalColor);
    OutDrawList.AddRect(m_Geometry, color, m_WorldMatrix);
}
