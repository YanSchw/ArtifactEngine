#include "UICurveEditor.h"
#include "EditorStyle.h"
#include "Particles/ParticleModules.h"
#include "Rendering/UIDrawList.h"

static constexpr float s_KeyRadius = 4.0f;
static constexpr float s_PickRadius = 8.0f;
static constexpr int32_t s_Samples = 64;

UICurveEditor::UICurveEditor() {
    Interactable = true;
    Cursor = CursorIcon::Hand;
}

UIRectF UICurveEditor::GetPlot() const {
    return m_Geometry.Deflate(UIPadding(s_KeyRadius + 3.0f));
}

Vec2 UICurveEditor::ToScreen(float InTime, float InValue) const {
    const UIRectF plot = GetPlot();
    const float alpha = (InValue - m_Range.x) / glm::max(m_Range.y - m_Range.x, 1e-4f);
    return Vec2(plot.Min().x + plot.Size.x * glm::clamp(InTime, 0.0f, 1.0f), plot.Max().y - plot.Size.y * alpha);
}

ParticleCurveKey UICurveEditor::KeyAt(const Vec2& InCursorPos) const {
    const UIRectF plot = GetPlot();
    const float time = (InCursorPos.x - plot.Min().x) / glm::max(plot.Size.x, 1.0f);
    const float alpha = (plot.Max().y - InCursorPos.y) / glm::max(plot.Size.y, 1.0f);
    return ParticleCurveKey(glm::clamp(time, 0.0f, 1.0f), glm::mix(m_Range.x, m_Range.y, glm::clamp(alpha, 0.0f, 1.0f)));
}

int32_t UICurveEditor::FindKey(const Vec2& InCursorPos) const {
    ParticleCurve* curve = GetCurve ? GetCurve() : nullptr;
    int32_t found = -1;
    float nearest = s_PickRadius;
    for (int32_t i = 0; curve && i < curve->Keys.Size(); i++) {
        const float distance = glm::distance(ToScreen(curve->Keys[i].Time, curve->Keys[i].Value), InCursorPos);
        if (distance < nearest) {
            nearest = distance;
            found = i;
        }
    }
    return found;
}

void UICurveEditor::UpdateRange() {
    m_Range = Vec2(0.0f, 1.0f);
    if (ParticleCurve* curve = GetCurve ? GetCurve() : nullptr) {
        for (const ParticleCurveKey& key : curve->Keys) {
            m_Range = Vec2(glm::min(m_Range.x, key.Value), glm::max(m_Range.y, key.Value));
        }
    }
}

void UICurveEditor::Edit(const std::function<void(ParticleCurve&)>& InEdit) {
    ParticleCurve* curve = GetCurve ? GetCurve() : nullptr;
    if (!curve) {
        return;
    }
    if (BeginChange) {
        BeginChange();
    }
    InEdit(*curve);
    if (Changed) {
        Changed();
    }
}

void UICurveEditor::Paint(UIDrawList& OutDrawList) {
    OutDrawList.AddRoundedRect(m_Geometry, IsHovered() ? EditorStyle::Accent : EditorStyle::FieldBorder, 3.0f, m_WorldMatrix);
    OutDrawList.AddRoundedRect(m_Geometry.Deflate(UIPadding(1.0f)), EditorStyle::PanelDark, 2.0f, m_WorldMatrix);
    ParticleCurve* curve = GetCurve ? GetCurve() : nullptr;
    if (!curve) {
        return;
    }
    if (m_DraggedKey < 0) {
        UpdateRange();
    }

    for (float value : { 0.0f, 1.0f }) {
        OutDrawList.AddLine(ToScreen(0.0f, value), ToScreen(1.0f, value), 1.0f, EditorStyle::FieldBorder, m_WorldMatrix);
    }
    Vec2 previous = ToScreen(0.0f, curve->Evaluate(0.0f));
    for (int32_t i = 1; i <= s_Samples; i++) {
        const float time = (float)i / (float)s_Samples;
        const Vec2 point = ToScreen(time, curve->Evaluate(time));
        OutDrawList.AddLine(previous, point, 1.5f, EditorStyle::AccentBright, m_WorldMatrix);
        previous = point;
    }
    for (int32_t i = 0; i < curve->Keys.Size(); i++) {
        const Vec2 center = ToScreen(curve->Keys[i].Time, curve->Keys[i].Value);
        OutDrawList.AddCircle(center, s_KeyRadius + 1.0f, EditorStyle::Border, 16, m_WorldMatrix);
        OutDrawList.AddCircle(center, s_KeyRadius, i == m_DraggedKey ? EditorStyle::TextBright : EditorStyle::Text, 16, m_WorldMatrix);
    }
}

void UICurveEditor::OnPressed(const Vec2& InCursorPos) {
    UpdateRange();
    m_DraggedKey = FindKey(InCursorPos);
    if (m_DraggedKey < 0) {
        Edit([this, &InCursorPos](ParticleCurve& OutCurve) {
            OutCurve.Keys.Add(KeyAt(InCursorPos));
            m_DraggedKey = OutCurve.Keys.Last();
        });
    }
}

void UICurveEditor::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (m_DraggedKey < 0) {
        return;
    }
    Edit([this, &InCursorPos](ParticleCurve& OutCurve) {
        if (m_DraggedKey < OutCurve.Keys.Size()) {
            OutCurve.Keys[m_DraggedKey] = KeyAt(InCursorPos);
        }
    });
}

void UICurveEditor::OnReleased(bool InInside) {
    (void)InInside;
    m_DraggedKey = -1;
}

bool UICurveEditor::OnSecondaryClick(const Vec2& InCursorPos) {
    const int32_t key = FindKey(InCursorPos);
    if (key < 0) {
        return false;
    }
    Edit([key](ParticleCurve& OutCurve) { OutCurve.Keys.RemoveAt(key); });
    return true;
}
