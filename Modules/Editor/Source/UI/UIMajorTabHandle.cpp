#include "UIMajorTabHandle.h"
#include "EditorWindow.h"
#include "EditorStyle.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UILabel.h"
#include "Assets/Font.h"

static constexpr float s_IconInset = 9.0f;
static constexpr float s_IconSize = 15.0f;
static constexpr float s_IconTextGap = 7.0f;
static constexpr float s_RightPad = 12.0f;
static constexpr float s_CollapsedWidth = s_IconInset * 2.0f + s_IconSize;

UIMajorTabHandle::UIMajorTabHandle() {
    ClipChildren = true;

    m_Icon = Add<UISvg>();
    m_Icon->Anchor = m_Icon->Pivot = Vec2(0.0f, 0.5f);
    m_Icon->Position = Vec2(s_IconInset, 0.0f);
    m_Icon->Size = Vec2(s_IconSize, s_IconSize);

    m_Label = Add<UILabel>();
    m_Label->Anchor = m_Label->Pivot = Vec2(0.0f, 0.5f);
    m_Label->Position = Vec2(s_IconInset + s_IconSize + s_IconTextGap, 0.0f);
    m_Label->FontSize = EditorStyle::FontSize;
    m_Label->VAlign = UIVAlign::Middle;
    m_Label->Size = { 400.0_px, 1.0_rel };
}

void UIMajorTabHandle::SetContent(const String& InCaption, VectorImage* InIcon) {
    m_Label->Text = InCaption;
    m_Icon->Image = InIcon;
    Font* font = UINode::GetDefaultFont();
    m_TextWidth = font ? font->MeasureText(InCaption, EditorStyle::FontSize).x : (float)InCaption.size() * 7.0f;
}

float UIMajorTabHandle::ExpandedWidth() const {
    return s_IconInset + s_IconSize + s_IconTextGap + m_TextWidth + s_RightPad;
}

void UIMajorTabHandle::OnUIUpdate(const UIFrameContext& InContext) {
    const float target = (IsHovered() || m_Active) ? 1.0f : 0.0f;
    m_Expand += (target - m_Expand) * glm::min(1.0f, InContext.DeltaTime * 16.0f);
    const float t = m_Expand * m_Expand * (3.0f - 2.0f * m_Expand);

    Size = { UIValue(s_CollapsedWidth + (ExpandedWidth() - s_CollapsedWidth) * t), 1.0_rel };

    NormalColor = m_Active ? EditorStyle::TabActive : EditorStyle::BottomBar;
    HoverColor = m_Active ? EditorStyle::TabActive : EditorStyle::TabHover;
    PressedColor = EditorStyle::ButtonPressed;

    m_Icon->Tint = m_Active ? EditorStyle::TextBright : EditorStyle::Text;
    const Vec4 base = m_Active ? EditorStyle::TextBright : EditorStyle::TextDim;
    m_Label->Color = Vec4(base.r, base.g, base.b, base.a * t);
}

void UIMajorTabHandle::OnPressed(const Vec2& InCursorPos) {
    m_PressPosition = InCursorPos;
}

void UIMajorTabHandle::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (!Tab || !OwnerWindow) {
        return;
    }
    if (OwnerWindow->IsDraggingTabHandle()) {
        OwnerWindow->UpdateTabHandleDrag(InCursorPos);
    } else if (glm::length(InCursorPos - m_PressPosition) > 8.0f) {
        OwnerWindow->BeginTabHandleDrag(Tab);
        OwnerWindow->UpdateTabHandleDrag(InCursorPos);
    }
}

void UIMajorTabHandle::OnReleased(bool InInside) {
    (void)InInside;
    if (Tab && OwnerWindow && OwnerWindow->IsDraggingTabHandle()) {
        OwnerWindow->EndTabHandleDrag(Tab);
    }
}
