#include "UIColorSlider.h"

#include "UIColorSwatch.h"
#include "EditorStyle.h"
#include "Rendering/UIDrawList.h"

UIColorSlider::UIColorSlider() {
    Interactable = true;
    Cursor = CursorIcon::Hand;
    Size = Vec2(22.0f, 180.0f);
}

void UIColorSlider::Paint(UIDrawList& OutDrawList) {
    const UIRectF inner = m_Geometry.Deflate(UIPadding(1.0f));

    OutDrawList.AddRoundedRect(m_Geometry, IsHovered() ? EditorStyle::Accent : EditorStyle::FieldBorder, 3.0f, m_WorldMatrix);
    if (Translucent) {
        UIColorSwatch::PaintColor(OutDrawList, inner, Color(0.0f), 2.0f, m_WorldMatrix);
    }
    OutDrawList.AddRoundedRectEx(inner, TopColor, BottomColor, 2.0f, 2.0f, 2.0f, 2.0f, m_WorldMatrix);

    const float y = inner.Min().y + inner.Size.y * (1.0f - glm::clamp(Value, 0.0f, 1.0f));
    const UIRectF handle(Vec2(m_Geometry.Min().x - 2.0f, y - 2.0f), Vec2(m_Geometry.Size.x + 4.0f, 4.0f));
    OutDrawList.AddRoundedRect(handle, Vec4(0.0f, 0.0f, 0.0f, 0.8f), 2.0f, m_WorldMatrix);
    OutDrawList.AddRoundedRect(handle.Deflate(UIPadding(1.0f)), EditorStyle::TextBright, 1.0f, m_WorldMatrix);
}

void UIColorSlider::PickAt(const Vec2& InCursorPos) {
    if (m_Geometry.Size.y <= 0.0f) {
        return;
    }
    Value = glm::clamp(1.0f - (InCursorPos.y - m_Geometry.Min().y) / m_Geometry.Size.y, 0.0f, 1.0f);
    if (Changed) {
        Changed();
    }
}

void UIColorSlider::OnPressed(const Vec2& InCursorPos) {
    PickAt(InCursorPos);
}

void UIColorSlider::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    PickAt(InCursorPos);
}
