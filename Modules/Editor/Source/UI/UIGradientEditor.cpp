#include "UIGradientEditor.h"
#include "EditorStyle.h"
#include "UIColorSwatch.h"
#include "Particles/ParticleModules.h"
#include "Rendering/UIDrawList.h"

#include <algorithm>

static constexpr float s_MarkerWidth = 7.0f;
static constexpr float s_MarkerHeight = 9.0f;
static constexpr int32_t s_Samples = 48;

UIGradientEditor::UIGradientEditor() {
    Interactable = true;
    Cursor = CursorIcon::Hand;
}

UIRectF UIGradientEditor::GetBar() const {
    const float inset = s_MarkerWidth * 0.5f;
    return UIRectF(m_Geometry.Min() + Vec2(inset, 0.0f), m_Geometry.Size - Vec2(inset * 2.0f, s_MarkerHeight));
}

float UIGradientEditor::TimeAt(const Vec2& InCursorPos) const {
    const UIRectF bar = GetBar();
    return glm::clamp((InCursorPos.x - bar.Min().x) / glm::max(bar.Size.x, 1.0f), 0.0f, 1.0f);
}

int32_t UIGradientEditor::FindKey(const Vec2& InCursorPos) const {
    ParticleGradient* gradient = GetGradient ? GetGradient() : nullptr;
    const UIRectF bar = GetBar();
    int32_t found = -1;
    float nearest = s_MarkerWidth;
    for (int32_t i = 0; gradient && i < gradient->Keys.Size(); i++) {
        const float distance = std::abs(bar.Min().x + bar.Size.x * glm::clamp(gradient->Keys[i].Time, 0.0f, 1.0f) - InCursorPos.x);
        if (distance < nearest) {
            nearest = distance;
            found = i;
        }
    }
    return found;
}

void UIGradientEditor::Edit(const std::function<void(ParticleGradient&)>& InEdit) {
    ParticleGradient* gradient = GetGradient ? GetGradient() : nullptr;
    if (!gradient) {
        return;
    }
    if (BeginChange) {
        BeginChange();
    }
    InEdit(*gradient);
    if (Changed) {
        Changed();
    }
}

void UIGradientEditor::Paint(UIDrawList& OutDrawList) {
    ParticleGradient* gradient = GetGradient ? GetGradient() : nullptr;
    const UIRectF bar = GetBar();
    OutDrawList.AddRoundedRect(bar, IsHovered() ? EditorStyle::Accent : EditorStyle::FieldBorder, 3.0f, m_WorldMatrix);
    const UIRectF inner = bar.Deflate(UIPadding(1.0f));
    UIColorSwatch::PaintColor(OutDrawList, inner, Color(0.0f), 2.0f, m_WorldMatrix);
    if (!gradient) {
        return;
    }

    Vec2 positions[(s_Samples + 1) * 2];
    Vec4 colors[(s_Samples + 1) * 2];
    uint32_t indices[s_Samples * 6];
    for (int32_t i = 0; i <= s_Samples; i++) {
        const float time = (float)i / (float)s_Samples;
        positions[i * 2] = Vec2(inner.Size.x * time, 0.0f);
        positions[i * 2 + 1] = Vec2(inner.Size.x * time, inner.Size.y);
        colors[i * 2] = colors[i * 2 + 1] = gradient->Evaluate(time);
    }
    for (uint32_t i = 0; i < (uint32_t)s_Samples; i++) {
        const uint32_t quad[6] = { i * 2, i * 2 + 1, i * 2 + 3, i * 2, i * 2 + 3, i * 2 + 2 };
        std::copy(quad, quad + 6, indices + i * 6);
    }
    OutDrawList.AddTriangles(positions, colors, (s_Samples + 1) * 2, indices, s_Samples * 6, Vec4(1.0f), inner.Min(), Vec2(1.0f), m_WorldMatrix);

    for (int32_t i = 0; i < gradient->Keys.Size(); i++) {
        const ParticleGradientKey& key = gradient->Keys[i];
        const float x = bar.Min().x + bar.Size.x * glm::clamp(key.Time, 0.0f, 1.0f);
        const UIRectF marker(Vec2(x - s_MarkerWidth * 0.5f, bar.Max().y + 1.0f), Vec2(s_MarkerWidth, s_MarkerHeight - 1.0f));
        OutDrawList.AddRoundedRect(marker, i == m_DraggedKey ? EditorStyle::TextBright : EditorStyle::Border, 2.0f, m_WorldMatrix);
        OutDrawList.AddRoundedRect(marker.Deflate(UIPadding(1.0f)), Color(Vec3(key.Value), 1.0f), 1.0f, m_WorldMatrix);
    }
}

void UIGradientEditor::OnPressed(const Vec2& InCursorPos) {
    m_DraggedKey = FindKey(InCursorPos);
    if (m_DraggedKey < 0) {
        const float time = TimeAt(InCursorPos);
        Edit([this, time](ParticleGradient& OutGradient) {
            OutGradient.Keys.Add(ParticleGradientKey(time, OutGradient.Evaluate(time)));
            m_DraggedKey = OutGradient.Keys.Last();
        });
    }
}

void UIGradientEditor::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (m_DraggedKey < 0) {
        return;
    }
    const float time = TimeAt(InCursorPos);
    Edit([this, time](ParticleGradient& OutGradient) {
        if (m_DraggedKey < OutGradient.Keys.Size()) {
            OutGradient.Keys[m_DraggedKey].Time = time;
        }
    });
}

void UIGradientEditor::OnReleased(bool InInside) {
    (void)InInside;
    m_DraggedKey = -1;
}

bool UIGradientEditor::OnSecondaryClick(const Vec2& InCursorPos) {
    const int32_t key = FindKey(InCursorPos);
    if (key < 0) {
        return false;
    }
    Edit([key](ParticleGradient& OutGradient) { OutGradient.Keys.RemoveAt(key); });
    return true;
}
