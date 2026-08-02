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
    m_PressPos = InCursorPos;
    m_Dragging = false;
    m_DoublePending = m_DoubleClickTimer > 0.0f;
    m_DoubleClickTimer = s_DoubleClickTime;
}

void ContentTile::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (!m_Dragging) {
        const Vec2 moved = InCursorPos - m_PressPos;
        if (moved.x * moved.x + moved.y * moved.y < s_DragThresholdSq) {
            return;
        }
        m_Dragging = true;
        if (DragStarted) {
            DragStarted();
        }
    }
    if (DragMoved) {
        DragMoved(InCursorPos);
    }
}

void ContentTile::OnReleased(bool InInside) {
    (void)InInside;
    if (m_Dragging && DragEnded) {
        DragEnded();
    }
    m_Dragging = false;
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
