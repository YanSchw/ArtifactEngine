#include "UIColorWheel.h"

#include "EditorStyle.h"
#include "Common/Color.h"
#include "Rendering/UIDrawList.h"

static constexpr int32_t s_Segments = 72;

UIColorWheel::UIColorWheel() {
    Interactable = true;
    Cursor = CursorIcon::Crosshair;
    Size = Vec2(180.0f);
}

void UIColorWheel::Paint(UIDrawList& OutDrawList) {
    const Vec2 center = GetCenter();
    const float radius = GetRadius();

    Vec2 positions[s_Segments + 2];
    Vec4 colors[s_Segments + 2];
    uint32_t indices[s_Segments * 3];

    positions[0] = center;
    colors[0] = Vec4(Value, Value, Value, 1.0f);
    for (int32_t i = 0; i <= s_Segments; i++) {
        const float angle = (float)i / (float)s_Segments * glm::two_pi<float>();
        positions[i + 1] = center + Vec2(glm::cos(angle), glm::sin(angle)) * radius;
        colors[i + 1] = Vec4(ColorUtils::HsvToRgb(Vec3(glm::degrees(angle), 1.0f, Value)), 1.0f);
    }
    for (int32_t i = 0; i < s_Segments; i++) {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = (uint32_t)i + 2;
        indices[i * 3 + 2] = (uint32_t)i + 1;
    }

    OutDrawList.AddTriangles(positions, colors, s_Segments + 2, indices, s_Segments * 3,
                             Vec4(1.0f), Vec2(0.0f), Vec2(1.0f), m_WorldMatrix);

    const float angle = glm::radians(Hue);
    const Vec2 marker = center + Vec2(glm::cos(angle), glm::sin(angle)) * (radius * glm::clamp(Saturation, 0.0f, 1.0f));
    OutDrawList.AddRing(marker, 5.0f, 1.5f, Vec4(0.0f, 0.0f, 0.0f, 0.8f), 20, m_WorldMatrix);
    OutDrawList.AddRing(marker, 6.5f, 1.5f, EditorStyle::TextBright, 20, m_WorldMatrix);
}

void UIColorWheel::PickAt(const Vec2& InCursorPos) {
    const Vec2 offset = InCursorPos - GetCenter();
    const float radius = GetRadius();
    if (radius <= 0.0f) {
        return;
    }

    Hue = glm::degrees(glm::atan(offset.y, offset.x));
    if (Hue < 0.0f) {
        Hue += 360.0f;
    }
    Saturation = glm::min(glm::length(offset) / radius, 1.0f);

    if (Changed) {
        Changed();
    }
}

void UIColorWheel::OnPressed(const Vec2& InCursorPos) {
    PickAt(InCursorPos);
}

void UIColorWheel::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    PickAt(InCursorPos);
}
