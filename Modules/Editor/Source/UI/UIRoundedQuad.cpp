#include "UIRoundedQuad.h"
#include "Rendering/UIDrawList.h"

void UIRoundedQuad::Paint(UIDrawList& OutDrawList) {
    if (Color.a <= 0.0f) {
        return;
    }
    if (CornerRadius == Vec4(0.0f)) {
        OutDrawList.AddRect(m_Geometry, Color, m_WorldMatrix);
    } else {
        OutDrawList.AddRoundedRectEx(m_Geometry, Color, Color,
                                     CornerRadius.x, CornerRadius.y, CornerRadius.z, CornerRadius.w,
                                     m_WorldMatrix);
    }
}
