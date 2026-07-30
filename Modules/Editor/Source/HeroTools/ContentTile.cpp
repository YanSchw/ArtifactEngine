#include "ContentTile.h"
#include "Rendering/UIDrawList.h"

void ContentTile::Paint(UIDrawList& OutDrawList) {
    const Vec4 color = (IsPressed() && IsHovered()) ? PressedColor : (IsHovered() ? HoverColor : NormalColor);
    if (color.a <= 0.0f) {
        return;
    }
    if (CornerRadius > 0.0f) {
        OutDrawList.AddRoundedRect(m_Geometry, color, CornerRadius, m_WorldMatrix);
    } else {
        OutDrawList.AddRect(m_Geometry, color, m_WorldMatrix);
    }
}

void ContentTile::OnPressed(const Vec2& InCursorPos) {
    (void)InCursorPos;
    m_DoublePending = m_DoubleClickTimer > 0.0f;
    m_DoubleClickTimer = s_DoubleClickTime;
}

void ContentTile::OnClick() {
    UIButton::OnClick();
    if (m_DoublePending && DoubleClicked) {
        DoubleClicked();
    }
    m_DoublePending = false;
}

void ContentTile::OnUIUpdate(const UIFrameContext& InContext) {
    if (m_DoubleClickTimer > 0.0f) {
        m_DoubleClickTimer -= InContext.DeltaTime;
    }
}
