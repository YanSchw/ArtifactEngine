#include "UITextArea.h"
#include "Rendering/UIDrawList.h"
#include "Assets/Font.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/Clipboard.h"
#include <cmath>

static constexpr float s_RepeatDelay = 0.45f;    // seconds a key is held before it repeats
static constexpr float s_RepeatInterval = 0.03f; // seconds between repeats once repeating
static constexpr float s_BlinkPeriod = 1.0f;

UITextArea::UITextArea() {
    Interactable = true;
    Cursor = CursorIcon::Text;
}

Font* UITextArea::ResolveFont() const {
    return FontAsset ? FontAsset : GetDefaultFont();
}

float UITextArea::LineHeight(Font* InFont) const {
    return InFont->GetLineHeightBasePx() * InFont->GetScaleForPixelHeight(FontSize);
}

float UITextArea::TextTop(Font* InFont) const {
    const UIRectF content = GetContentRect();
    if (SingleLine) {
        return content.Min().y + (content.Size.y - LineHeight(InFont)) * 0.5f;
    }
    return content.Min().y;
}

void UITextArea::SetCaret(int32_t InIndex) {
    m_SelectionAnchor = -1;
    m_Caret = std::clamp(InIndex, 0, (int32_t)Text.size());
    m_CaretBlink = 0.0f;
}

void UITextArea::SetSelection(int32_t InStart, int32_t InEnd) {
    m_SelectionAnchor = std::clamp(InStart, 0, (int32_t)Text.size());
    m_Caret = std::clamp(InEnd, 0, (int32_t)Text.size());
    m_CaretBlink = 0.0f;
}

bool UITextArea::GetSelection(int32_t& OutStart, int32_t& OutEnd) const {
    if (!HasSelection()) {
        return false;
    }
    OutStart = std::min(m_SelectionAnchor, m_Caret);
    OutEnd = std::max(m_SelectionAnchor, m_Caret);
    return true;
}

void UITextArea::MoveCaret(int32_t InIndex, bool InExtendSelection) {
    if (InExtendSelection) {
        if (m_SelectionAnchor < 0) {
            m_SelectionAnchor = m_Caret;
        }
    } else {
        m_SelectionAnchor = -1;
    }
    m_Caret = std::clamp(InIndex, 0, (int32_t)Text.size());
    m_CaretBlink = 0.0f;
}

bool UITextArea::DeleteSelection() {
    int32_t start = 0, end = 0;
    if (!GetSelection(start, end)) {
        m_SelectionAnchor = -1;
        return false;
    }
    Text.erase((size_t)start, (size_t)(end - start));
    m_Caret = start;
    m_SelectionAnchor = -1;
    NotifyEdited();
    return true;
}

void UITextArea::InsertAtCaret(const String& InText) {
    DeleteSelection();
    m_Caret = std::clamp(m_Caret, 0, (int32_t)Text.size());
    Text.insert((size_t)m_Caret, InText);
    m_Caret += (int32_t)InText.size();
    NotifyEdited();
}

String UITextArea::SanitizeInput(const String& InText) const {
    String result;
    result.reserve(InText.size());
    for (char c : InText) {
        if (c == '\r') {
            continue;
        }
        if ((c >= 32 && c < 127) || (c == '\n' && !SingleLine)) {
            result += c;
        }
    }
    return result;
}

void UITextArea::NotifyEdited() {
    m_CaretBlink = 0.0f;
    if (TextChanged) {
        TextChanged(Text);
    }
}

void UITextArea::GetLineBounds(int32_t InIndex, int32_t& OutStart, int32_t& OutEnd) const {
    OutStart = 0;
    if (InIndex > 0) {
        const size_t previousBreak = Text.rfind('\n', (size_t)InIndex - 1);
        OutStart = (previousBreak == String::npos) ? 0 : (int32_t)previousBreak + 1;
    }
    const size_t nextBreak = Text.find('\n', (size_t)InIndex);
    OutEnd = (nextBreak == String::npos) ? (int32_t)Text.size() : (int32_t)nextBreak;
}

int32_t UITextArea::LineOfIndex(int32_t InIndex) const {
    int32_t line = 0;
    for (int32_t i = 0; i < InIndex && i < (int32_t)Text.size(); i++) {
        if (Text[(size_t)i] == '\n') {
            line++;
        }
    }
    return line;
}

Vec2 UITextArea::CaretLocalPosition(Font* InFont) const {
    int32_t lineStart = 0, lineEnd = 0;
    GetLineBounds(m_Caret, lineStart, lineEnd);
    const float x = InFont->MeasureText(Text.substr((size_t)lineStart, (size_t)(m_Caret - lineStart)), FontSize).x;
    return Vec2(x, (float)LineOfIndex(m_Caret) * LineHeight(InFont));
}

Vec2 UITextArea::ScreenToLocal(const Vec2& InScreenPoint) const {
    // Invert the local->screen mapping from two basis probes. Exact for any affine transform
    // (canvas scale, z-rotation); a perspective-tilted field would need a full unprojection.
    const Vec2 origin = LocalToScreen(Vec2(0.0f));
    const Vec2 basisX = LocalToScreen(Vec2(1.0f, 0.0f)) - origin;
    const Vec2 basisY = LocalToScreen(Vec2(0.0f, 1.0f)) - origin;
    const float det = basisX.x * basisY.y - basisX.y * basisY.x;
    if (std::abs(det) < 1e-6f) {
        return Vec2(0.0f);
    }
    const Vec2 delta = InScreenPoint - origin;
    return Vec2((delta.x * basisY.y - delta.y * basisY.x) / det,
                (delta.y * basisX.x - delta.x * basisX.y) / det);
}

int32_t UITextArea::IndexFromPoint(const Vec2& InScreenPoint) const {
    Font* font = ResolveFont();
    if (!font) {
        return (int32_t)Text.size();
    }
    const Vec2 local = ScreenToLocal(InScreenPoint);

    int32_t lineStart = 0, lineEnd = 0;
    GetLineBounds(0, lineStart, lineEnd);
    const int32_t targetLine = std::max(0, (int32_t)std::floor((local.y - TextTop(font)) / LineHeight(font)));
    for (int32_t line = 0; line < targetLine && lineEnd < (int32_t)Text.size(); line++) {
        GetLineBounds(lineEnd + 1, lineStart, lineEnd);
    }

    // Walk the line's advances; the caret lands on whichever side of the glyph the click hit.
    const float scale = font->GetScaleForPixelHeight(FontSize);
    float penX = GetContentRect().Min().x - m_ScrollX;
    int32_t index = lineStart;
    while (index < lineEnd) {
        GlyphInfo glyph;
        const float advance = font->GetGlyph((uint32_t)(unsigned char)Text[(size_t)index], glyph)
            ? glyph.AdvancePx * scale : 0.0f;
        if (local.x < penX + advance * 0.5f) {
            break;
        }
        penX += advance;
        index++;
    }
    return index;
}

static bool IsAnyKeyHeld(KeyboardDevice* InKeyboard, KeyCode InLeft, KeyCode InRight) {
    return InKeyboard && (InKeyboard->IsPressed(InLeft) || InKeyboard->IsPressed(InRight));
}

static bool IsShiftHeld() {
    return IsAnyKeyHeld(KeyboardDevice::Instance(), KeyCode::LeftShift, KeyCode::RightShift);
}

void UITextArea::OnPressed(const Vec2& InCursorPos) {
    // Shift+click extends the selection; a plain click collapses it and re-anchors for dragging.
    MoveCaret(IndexFromPoint(InCursorPos), IsShiftHeld());
    if (m_SelectionAnchor < 0) {
        m_SelectionAnchor = m_Caret;
    }
}

void UITextArea::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    MoveCaret(IndexFromPoint(InCursorPos), true);
}

void UITextArea::OnFocusChanged(bool InFocused) {
    m_CaretBlink = 0.0f;
    if (!InFocused && FocusLost) {
        FocusLost();
    }
}

void UITextArea::OnUIUpdate(const UIFrameContext& InContext) {
    m_CaretBlink += InContext.DeltaTime;
    if (!IsFocused()) {
        return;
    }
    m_Caret = std::clamp(m_Caret, 0, (int32_t)Text.size());
    m_SelectionAnchor = std::min(m_SelectionAnchor, (int32_t)Text.size());

    if (!InContext.TextInput.empty()) {
        InsertAtCaret(SanitizeInput(InContext.TextInput));
    }
    if (KeyboardDevice* keyboard = KeyboardDevice::Instance()) {
        HandleEditKeys(keyboard, InContext.DeltaTime);
    }
    ScrollCaretIntoView();
}

bool UITextArea::PollEditKey(KeyboardDevice* InKeyboard, KeyCode InKey, float InDeltaTime) {
    if (InKeyboard->IsDown(InKey)) {
        m_RepeatKey = InKey;
        m_RepeatHeld = 0.0f;
        return true;
    }
    if (InKey == m_RepeatKey && InKeyboard->IsPressed(InKey)) {
        m_RepeatHeld += InDeltaTime;
        if (m_RepeatHeld >= s_RepeatDelay) {
            m_RepeatHeld -= s_RepeatInterval;
            return true;
        }
    }
    return false;
}

void UITextArea::HandleEditKeys(KeyboardDevice* InKeyboard, float InDeltaTime) {
    if (InKeyboard->IsDown(KeyCode::Escape)) {
        if (Cancelled) {
            Cancelled();
        }
        ReleaseFocus();
        return;
    }
    if (IsAnyKeyHeld(InKeyboard, KeyCode::LeftControl, KeyCode::RightControl)
        || IsAnyKeyHeld(InKeyboard, KeyCode::LeftSuper, KeyCode::RightSuper)) {
        HandleClipboardShortcuts(InKeyboard);
        return;
    }
    if (PollEditKey(InKeyboard, KeyCode::Enter, InDeltaTime) || PollEditKey(InKeyboard, KeyCode::KPEnter, InDeltaTime)) {
        if (SingleLine) {
            if (Submitted) {
                Submitted(Text);
            }
            ReleaseFocus();
            return;
        }
        InsertAtCaret("\n");
    }

    if (PollEditKey(InKeyboard, KeyCode::Backspace, InDeltaTime) && !DeleteSelection() && m_Caret > 0) {
        Text.erase((size_t)m_Caret - 1, 1);
        m_Caret--;
        NotifyEdited();
    }
    if (PollEditKey(InKeyboard, KeyCode::Delete, InDeltaTime) && !DeleteSelection() && m_Caret < (int32_t)Text.size()) {
        Text.erase((size_t)m_Caret, 1);
        NotifyEdited();
    }

    const bool shift = IsShiftHeld();
    int32_t selectionStart = 0, selectionEnd = 0;
    if (PollEditKey(InKeyboard, KeyCode::Left, InDeltaTime)) {
        // Without shift, an arrow first collapses the selection to its edge.
        if (!shift && GetSelection(selectionStart, selectionEnd)) {
            SetCaret(selectionStart);
        } else {
            MoveCaret(m_Caret - 1, shift);
        }
    }
    if (PollEditKey(InKeyboard, KeyCode::Right, InDeltaTime)) {
        if (!shift && GetSelection(selectionStart, selectionEnd)) {
            SetCaret(selectionEnd);
        } else {
            MoveCaret(m_Caret + 1, shift);
        }
    }
    int32_t lineStart = 0, lineEnd = 0;
    if (InKeyboard->IsDown(KeyCode::Home)) {
        GetLineBounds(m_Caret, lineStart, lineEnd);
        MoveCaret(lineStart, shift);
    }
    if (InKeyboard->IsDown(KeyCode::End)) {
        GetLineBounds(m_Caret, lineStart, lineEnd);
        MoveCaret(lineEnd, shift);
    }
    if (!SingleLine) {
        if (PollEditKey(InKeyboard, KeyCode::Up, InDeltaTime)) {
            MoveCaretLine(-1, shift);
        }
        if (PollEditKey(InKeyboard, KeyCode::Down, InDeltaTime)) {
            MoveCaretLine(1, shift);
        }
    }
}

void UITextArea::HandleClipboardShortcuts(KeyboardDevice* InKeyboard) {
    if (InKeyboard->IsDown(KeyCode::A)) {
        SelectAll();
    }
    int32_t start = 0, end = 0;
    if ((InKeyboard->IsDown(KeyCode::C) || InKeyboard->IsDown(KeyCode::X)) && GetSelection(start, end)) {
        Clipboard::SetText(Text.substr((size_t)start, (size_t)(end - start)));
        if (InKeyboard->IsDown(KeyCode::X)) {
            DeleteSelection();
        }
    }
    if (InKeyboard->IsDown(KeyCode::V)) {
        const String pasted = SanitizeInput(Clipboard::GetText());
        if (!pasted.empty()) {
            InsertAtCaret(pasted);
        }
    }
}

void UITextArea::MoveCaretLine(int32_t InDirection, bool InExtendSelection) {
    int32_t lineStart = 0, lineEnd = 0;
    GetLineBounds(m_Caret, lineStart, lineEnd);
    const int32_t column = m_Caret - lineStart;
    if (InDirection < 0) {
        if (lineStart == 0) {
            MoveCaret(0, InExtendSelection);
            return;
        }
        int32_t previousStart = 0, previousEnd = 0;
        GetLineBounds(lineStart - 1, previousStart, previousEnd);
        MoveCaret(previousStart + std::min(column, previousEnd - previousStart), InExtendSelection);
    } else {
        if (lineEnd >= (int32_t)Text.size()) {
            MoveCaret((int32_t)Text.size(), InExtendSelection);
            return;
        }
        int32_t nextStart = 0, nextEnd = 0;
        GetLineBounds(lineEnd + 1, nextStart, nextEnd);
        MoveCaret(nextStart + std::min(column, nextEnd - nextStart), InExtendSelection);
    }
}

void UITextArea::ScrollCaretIntoView() {
    Font* font = ResolveFont();
    if (!font || !SingleLine) {
        m_ScrollX = 0.0f;
        return;
    }
    const float caretX = CaretLocalPosition(font).x;
    const float width = std::max(GetContentRect().Size.x, 1.0f);
    m_ScrollX = std::clamp(m_ScrollX, caretX - width + 2.0f, caretX);
    m_ScrollX = std::max(0.0f, m_ScrollX);
}

void UITextArea::Paint(UIDrawList& OutDrawList) {
    if (IsFocused() && FocusedBorderColor.a > 0.0f) {
        const UIRectF border(m_Geometry.Position - Vec2(1.0f), m_Geometry.Size + Vec2(2.0f));
        OutDrawList.AddRoundedRect(border, FocusedBorderColor, CornerRadius + 1.0f, m_WorldMatrix);
    }
    if (BackgroundColor.a > 0.0f) {
        OutDrawList.AddRoundedRect(m_Geometry, BackgroundColor, CornerRadius, m_WorldMatrix);
    }

    Font* font = ResolveFont();
    if (!font) {
        return;
    }

    const UIRectF content = GetContentRect();
    OutDrawList.PushClipRect(content);

    const float lineHeight = LineHeight(font);
    const float top = TextTop(font);
    const float left = content.Min().x - m_ScrollX;

    if (Text.empty() && !Placeholder.empty() && !IsFocused()) {
        OutDrawList.AddText(font, Placeholder, Vec2(left, top), FontSize, PlaceholderColor, m_WorldMatrix);
    }

    int32_t selectionStart = 0, selectionEnd = 0;
    const bool selecting = IsFocused() && GetSelection(selectionStart, selectionEnd);

    int32_t lineStart = 0;
    int32_t lineIndex = 0;
    while (lineStart <= (int32_t)Text.size()) {
        const size_t lineBreak = Text.find('\n', (size_t)lineStart);
        const int32_t lineEnd = (lineBreak == String::npos) ? (int32_t)Text.size() : (int32_t)lineBreak;
        const String line = Text.substr((size_t)lineStart, (size_t)(lineEnd - lineStart));
        const float lineY = top + (float)lineIndex * lineHeight;

        if (selecting && selectionStart <= lineEnd && selectionEnd >= lineStart) {
            const int32_t from = std::max(selectionStart, lineStart);
            const int32_t to = std::min(selectionEnd, lineEnd);
            const float x0 = font->MeasureText(line.substr(0, (size_t)(from - lineStart)), FontSize).x;
            float x1 = font->MeasureText(line.substr(0, (size_t)(to - lineStart)), FontSize).x;
            if (selectionEnd > lineEnd) {
                x1 += lineHeight * 0.3f;  // mark the selected line break past the line's end
            }
            if (x1 > x0) {
                OutDrawList.AddRect(UIRectF(Vec2(left + x0, lineY), Vec2(x1 - x0, lineHeight)),
                                    SelectionColor, m_WorldMatrix);
            }
        }

        OutDrawList.AddText(font, line, Vec2(left, lineY), FontSize, TextColor, m_WorldMatrix);
        if (lineBreak == String::npos) {
            break;
        }
        lineStart = lineEnd + 1;
        lineIndex++;
    }

    if (IsFocused() && std::fmod(m_CaretBlink, s_BlinkPeriod) < s_BlinkPeriod * 0.5f) {
        const Vec2 caret = CaretLocalPosition(font);
        const UIRectF caretRect(Vec2(left + caret.x, top + caret.y + 1.0f), Vec2(1.5f, lineHeight - 2.0f));
        OutDrawList.AddRect(caretRect, CaretColor, m_WorldMatrix);
    }

    OutDrawList.PopClipRect();
}
