#include "UIQuad.h"
#include "Rendering/UIDrawList.h"

void UIQuad::Paint(UIDrawList& OutDrawList) {
    if (Color.a > 0.0f) {
        OutDrawList.AddRect(m_Geometry, Color, m_WorldMatrix);
    }
}
