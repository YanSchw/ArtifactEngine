#pragma once
#include "UINode.h"
#include "InputSystem/KeyCodes.h"
#include <functional>
#include "UITextArea.gen.h"

class Font;
class KeyboardDevice;

/** An editable text box: caret with blink, click-to-place, drag/shift selection, clipboard
 *  shortcuts (Ctrl/Cmd + A/C/X/V), key-repeat editing, Enter/Escape handling and clipped
 *  rendering. Multiline by default; SingleLine turns it into a one-line text field where Enter
 *  fires Submitted (instead of inserting a line break) and the view scrolls horizontally to
 *  keep the caret visible. */
class UITextArea : public UINode {
public:
    ARTIFACT_CLASS();

    UITextArea();

    String Text;
    /** One-line field: Enter submits instead of inserting a line break. */
    bool SingleLine = false;
    /** Drawn dim while Text is empty and the field is not focused. */
    String Placeholder;
    Font* FontAsset = nullptr;  // falls back to UINode::GetDefaultFont()
    float FontSize = 18.0f;
    Vec4 TextColor = Vec4(1.0f);
    Vec4 PlaceholderColor = Vec4(1.0f, 1.0f, 1.0f, 0.3f);
    Vec4 CaretColor = Vec4(1.0f);
    Vec4 SelectionColor = Vec4(0.15f, 0.4f, 0.8f, 0.5f);
    Vec4 BackgroundColor = Vec4(0.0f, 0.0f, 0.0f, 0.35f);
    Vec4 FocusedBorderColor = Vec4(0.15f, 0.55f, 1.0f, 1.0f);
    float CornerRadius = 3.0f;

    std::function<void(const String&)> TextChanged;  // after every edit
    std::function<void(const String&)> Submitted;    // Enter on a SingleLine field
    std::function<void()> Cancelled;                 // Escape (the field also gives up focus)
    std::function<void()> FocusLost;

    /** Places the caret (clamped to [0, Text length]) and collapses any selection. */
    void SetCaret(int32_t InIndex);
    int32_t GetCaret() const { return m_Caret; }

    /** Selects [InStart, InEnd) and puts the caret at InEnd. */
    void SetSelection(int32_t InStart, int32_t InEnd);
    void SelectAll() { SetSelection(0, (int32_t)Text.size()); }
    bool HasSelection() const { return m_SelectionAnchor >= 0 && m_SelectionAnchor != m_Caret; }
    /** Ordered selection range; false when nothing is selected. */
    bool GetSelection(int32_t& OutStart, int32_t& OutEnd) const;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnFocusChanged(bool InFocused) override;

private:
    /** Moves the caret; extending keeps (or starts) a selection from the previous position. */
    void MoveCaret(int32_t InIndex, bool InExtendSelection);
    /** Removes the selected range, leaving the caret at its start. False when nothing was selected. */
    bool DeleteSelection();
    void InsertAtCaret(const String& InText);
    void NotifyEdited();
    void HandleEditKeys(KeyboardDevice* InKeyboard, float InDeltaTime);
    void HandleClipboardShortcuts(KeyboardDevice* InKeyboard);
    /** True when the key should act this frame: on its press edge, then key-repeat while held. */
    bool PollEditKey(KeyboardDevice* InKeyboard, KeyCode InKey, float InDeltaTime);
    void MoveCaretLine(int32_t InDirection, bool InExtendSelection);
    void ScrollCaretIntoView();
    /** Drops characters the field cannot represent (non-ASCII; line breaks when SingleLine). */
    String SanitizeInput(const String& InText) const;

    /** Byte range [OutStart, OutEnd) of the line containing InIndex (OutEnd excludes the '\n'). */
    void GetLineBounds(int32_t InIndex, int32_t& OutStart, int32_t& OutEnd) const;
    int32_t LineOfIndex(int32_t InIndex) const;
    /** Caret offset in content-local pixels: x from the line start, y = line * line height. */
    Vec2 CaretLocalPosition(Font* InFont) const;
    int32_t IndexFromPoint(const Vec2& InScreenPoint) const;
    Vec2 ScreenToLocal(const Vec2& InScreenPoint) const;
    float LineHeight(Font* InFont) const;
    /** Content-local y of line 0; a SingleLine field centers its line vertically. */
    float TextTop(Font* InFont) const;
    Font* ResolveFont() const;

    int32_t m_Caret = 0;
    int32_t m_SelectionAnchor = -1;  // selection spans anchor<->caret; -1 = none
    float m_CaretBlink = 0.0f;
    float m_ScrollX = 0.0f;
    KeyCode m_RepeatKey = KeyCode::Space;  // last edit key pressed; only repeats while held
    float m_RepeatHeld = 0.0f;
};
