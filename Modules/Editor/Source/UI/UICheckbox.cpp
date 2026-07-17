#include "UICheckbox.h"
#include "EditorStyle.h"
#include "Rendering/UIDrawList.h"

UICheckbox::UICheckbox() {
    Size = Vec2(18.0f);
}

void UICheckbox::Paint(UIDrawList& OutDrawList) {
    const float radius = 4.0f;
    const Vec4 border = IsHovered() ? EditorStyle::Accent : EditorStyle::FieldBorder;
    const Vec4 fill = IsHovered() ? EditorStyle::Panel : EditorStyle::PanelDark;

    OutDrawList.AddRoundedRect(m_Geometry, border, radius, m_WorldMatrix);
    OutDrawList.AddRoundedRect(m_Geometry.Deflate(UIPadding(1.0f)), fill, radius - 1.0f, m_WorldMatrix);

    if (IsOn) {
        const Vec2 origin = m_Geometry.Min();
        const Vec2 extent = m_Geometry.Size;
        const Vec2 p0 = origin + extent * Vec2(0.24f, 0.52f);
        const Vec2 p1 = origin + extent * Vec2(0.43f, 0.70f);
        const Vec2 p2 = origin + extent * Vec2(0.75f, 0.30f);
        OutDrawList.AddLine(p0, p1, 2.0f, EditorStyle::AccentBright, m_WorldMatrix);
        OutDrawList.AddLine(p1, p2, 2.0f, EditorStyle::AccentBright, m_WorldMatrix);
    }
}
