#include "UIDragNumber.h"
#include "EditorStyle.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UITextArea.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

UIDragNumber::UIDragNumber() {
    Size = { 1.0_rel, 20.0f };
    Interactable = true;
    Cursor = CursorIcon::ResizeH;

    m_PrefixLabel = Add<UILabel>();
    m_PrefixLabel->Anchor = m_PrefixLabel->Pivot = Vec2(0.0f, 0.5f);
    m_PrefixLabel->FontSize = EditorStyle::FontSize - 2.0f;
    m_PrefixLabel->Color = EditorStyle::TextBright;
    m_PrefixLabel->HAlign = UIHAlign::Center;
    m_PrefixLabel->VAlign = UIVAlign::Middle;

    m_ValueLabel = Add<UILabel>();
    m_ValueLabel->Anchor = m_ValueLabel->Pivot = Vec2(0.0f, 0.5f);
    m_ValueLabel->FontSize = EditorStyle::FontSize;
    m_ValueLabel->Color = EditorStyle::Text;
    m_ValueLabel->VAlign = UIVAlign::Middle;

    m_EditField = Add<UITextArea>();
    m_EditField->Anchor = m_EditField->Pivot = Vec2(0.0f, 0.5f);
    m_EditField->SingleLine = true;
    m_EditField->FontSize = EditorStyle::FontSize;
    m_EditField->TextColor = EditorStyle::TextBright;
    m_EditField->CaretColor = EditorStyle::TextBright;
    m_EditField->BackgroundColor = Vec4(0.0f);
    m_EditField->FocusedBorderColor = Vec4(0.0f);
    m_EditField->Padding = UIPadding(6.0f, 0.0f);
    m_EditField->SetEnabled(false);
    m_EditField->Submitted = [this](const String& InText) { CommitEdit(InText); };
    m_EditField->Cancelled = [this] { EndEdit(); };
    m_EditField->FocusLost = [this] {
        if (m_Editing) {
            CommitEdit(m_EditField->Text);
        }
    };
}

void UIDragNumber::OnBind() {
    UINode::OnBind();

    const float prefix = PrefixWidth();
    m_PrefixLabel->SetEnabled(prefix > 0.0f && !Prefix.empty());
    m_PrefixLabel->Text = Prefix;
    m_PrefixLabel->Size = { prefix, 1.0_rel };

    m_ValueLabel->Position = Vec2(prefix + 6.0f, 0.0f);
    m_ValueLabel->Size = { 1.0_rel - UIValue::Px(prefix + 12.0f), 1.0_rel };
    m_EditField->Position = Vec2(prefix, 0.0f);
    m_EditField->Size = { 1.0_rel - UIValue::Px(prefix), 1.0_rel };

    m_ValueLabel->SetEnabled(!m_Editing);
    if (!m_Editing && Get) {
        m_ValueLabel->Text = FormatValue(Get());
    }
}

void UIDragNumber::Paint(UIDrawList& OutDrawList) {
    const Vec4 border = m_Editing ? EditorStyle::Accent
                                  : (IsHovered() ? EditorStyle::FieldBorder : Vec4(0.0f));
    OutDrawList.AddRoundedRect(m_Geometry, EditorStyle::PanelDark, 2.0f, m_WorldMatrix);
    if (border.a > 0.0f) {
        const UIRectF top(m_Geometry.Min(), Vec2(m_Geometry.Size.x, 1.0f));
        const UIRectF bottom(m_Geometry.Min() + Vec2(0.0f, m_Geometry.Size.y - 1.0f), Vec2(m_Geometry.Size.x, 1.0f));
        const UIRectF left(m_Geometry.Min(), Vec2(1.0f, m_Geometry.Size.y));
        const UIRectF right(m_Geometry.Min() + Vec2(m_Geometry.Size.x - 1.0f, 0.0f), Vec2(1.0f, m_Geometry.Size.y));
        OutDrawList.AddRect(top, border, m_WorldMatrix);
        OutDrawList.AddRect(bottom, border, m_WorldMatrix);
        OutDrawList.AddRect(left, border, m_WorldMatrix);
        OutDrawList.AddRect(right, border, m_WorldMatrix);
    }
    if (PrefixWidth() > 0.0f) {
        const UIRectF chip(m_Geometry.Position, Vec2(PrefixWidth(), m_Geometry.Size.y));
        OutDrawList.AddRoundedRect(chip, PrefixColor, 2.0f, m_WorldMatrix);
    }
}

bool UIDragNumber::IsOverPrefix(const Vec2& InScreenPos) const {
    if (PrefixWidth() <= 0.0f) {
        return false;
    }
    const UIRectF chip(m_Geometry.Position, Vec2(PrefixWidth(), m_Geometry.Size.y));
    return HitTestRect(chip, InScreenPos);
}

void UIDragNumber::OnUIUpdate(const UIFrameContext& InContext) {
    Cursor = (!m_Editing && PrefixClicked && IsOverPrefix(InContext.CursorPosition)) ? CursorIcon::Hand : CursorIcon::ResizeH;
}

void UIDragNumber::OnPressed(const Vec2& InCursorPos) {
    if (m_Editing) {
        return;
    }
    m_PressPos = InCursorPos;
    m_Dragging = false;
    m_PressedOnPrefix = IsOverPrefix(InCursorPos);
    m_DragValue = Get ? Get() : 0.0;
}

void UIDragNumber::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    if (m_Editing) {
        return;
    }
    if (!m_Dragging && std::abs(InCursorPos.x - m_PressPos.x) < 3.0f) {
        return;
    }
    m_Dragging = true;
    m_DragValue += (double)InDelta.x * Sensitivity;
    WriteValue(m_DragValue);
}

void UIDragNumber::OnReleased(bool InInside) {
    if (!m_Dragging && InInside && !m_Editing) {
        if (m_PressedOnPrefix && PrefixClicked) {
            PrefixClicked();
        } else {
            BeginEdit();
        }
    }
    m_Dragging = false;
    m_PressedOnPrefix = false;
}

void UIDragNumber::BeginEdit() {
    m_Editing = true;
    m_EditField->SetEnabled(true);
    m_EditField->Text = Get ? FormatValue(Get()) : "";
    m_EditField->SelectAll();
    m_EditField->RequestFocus();
}

void UIDragNumber::CommitEdit(const String& InText) {
    char* end = nullptr;
    const double parsed = std::strtod(InText.c_str(), &end);
    if (end != InText.c_str()) {
        WriteValue(parsed);
    }
    EndEdit();
}

void UIDragNumber::EndEdit() {
    m_Editing = false;
    m_EditField->ReleaseFocus();
    m_EditField->SetEnabled(false);
}

void UIDragNumber::WriteValue(double InValue) {
    if (!Set) {
        return;
    }
    double value = std::clamp(InValue, MinValue, MaxValue);
    if (Integer) {
        value = std::round(value);
    }
    Set(value);
}

String UIDragNumber::FormatValue(double InValue) const {
    char buffer[32];
    if (Integer) {
        std::snprintf(buffer, sizeof(buffer), "%lld", (long long)std::llround(InValue));
    } else {
        const double scale = std::pow(10.0, (double)Decimals);
        const double rounded = std::round(InValue * scale) / scale + 0.0;  // + 0.0 turns -0 into 0
        std::snprintf(buffer, sizeof(buffer), "%.*f", (int)Decimals, rounded);
    }
    return buffer;
}
