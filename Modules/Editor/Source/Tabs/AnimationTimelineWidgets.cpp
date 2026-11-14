#include "AnimationTimelineWidgets.h"
#include "AnimationTimelineTab.h"
#include "AnimationEditorTab.h"
#include "MajorTab.h"
#include "Details/DetailsCustomization.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIContextMenu.h"
#include "UI/UIMenuModel.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UITextArea.h"
#include "UI/UITooltip.h"
#include "GameFramework/Node.h"
#include "Assets/Animation.h"
#include "Assets/Font.h"
#include "Rendering/UIDrawList.h"
#include "InputSystem/KeyboardDevice.h"
#include <algorithm>
#include <cmath>

static const Vec4 s_RowEven = HexColor(0x232323);
static const Vec4 s_RowOdd = HexColor(0x272727);
static const Vec4 s_RowSelected = HexColor(0x2E3B48);
static const Vec4 s_Grid = Vec4(1.0f, 1.0f, 1.0f, 0.05f);
static const Vec4 s_KeyColor = HexColor(0xC8C8C8);
static const Vec4 s_KeyStepColor = HexColor(0x8F8F8F);
static const Vec4 s_PlayHead = HexColor(0x4AA3FF);
static const Vec4 s_ScrollTrack = HexColor(0x141414);
static const Vec4 s_ScrollThumb = HexColor(0x4A4A4A);
static const Vec4 s_ScrollThumbHover = HexColor(0x6A6A6A);
static const Vec4 s_OutOfRange = Vec4(0.0f, 0.0f, 0.0f, 0.35f);
static const Vec4 s_OutOfRangeText = HexColor(0x585858);
static const Vec4 s_EventColor = HexColor(0xE8E8E8);

static constexpr float s_KeyRadius = 5.0f;
static constexpr float s_MinThumbWidth = 16.0f;

static bool IsShiftHeld() {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && (keyboard->IsPressed(KeyCode::LeftShift) || keyboard->IsPressed(KeyCode::RightShift));
}

static bool IsZoomModifierHeld() {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && (keyboard->IsPressed(KeyCode::LeftControl) || keyboard->IsPressed(KeyCode::RightControl)
                     || keyboard->IsPressed(KeyCode::LeftSuper) || keyboard->IsPressed(KeyCode::RightSuper));
}

static void PaintDiamond(UIDrawList& OutDrawList, const Vec2& InCenter, float InRadius, const Vec4& InColor, const Mat4& InTransform) {
    const Vec2 points[4] = {
        Vec2(InCenter.x - InRadius, InCenter.y),
        Vec2(InCenter.x, InCenter.y + InRadius),
        Vec2(InCenter.x + InRadius, InCenter.y),
        Vec2(InCenter.x, InCenter.y - InRadius),
    };
    OutDrawList.AddConvexPolyFilled(points, 4, InColor, InTransform);
}

/* -------------------------------- Strip -------------------------------- */

AnimationTimelineStrip::AnimationTimelineStrip() {
    Interactable = true;
}

UIRectF AnimationTimelineStrip::Strip() const {
    return Owner ? Owner->StripOf(m_Geometry) : m_Geometry;
}

bool AnimationTimelineStrip::IsOverLabelColumn(const Vec2& InCursorPos) const {
    return Owner && InCursorPos.x < m_Geometry.Min().x + Owner->GetLabelWidth();
}

void AnimationTimelineStrip::OnUIUpdate(const UIFrameContext& InContext) {
    m_Cursor = InContext.CursorPosition;
}

bool AnimationTimelineStrip::OnScroll(const Vec2& InDelta) {
    if (!Owner) {
        return false;
    }
    // A horizontal wheel (or shift-wheel) pans; ctrl-wheel always zooms, a plain wheel only
    // where nothing else needs it, so the track list keeps its vertical scrolling.
    const float pan = InDelta.x != 0.0f ? InDelta.x : (IsShiftHeld() ? InDelta.y : 0.0f);
    if (pan != 0.0f) {
        Owner->PanByPixels(Strip(), pan * 24.0f);
        return true;
    }
    if (InDelta.y != 0.0f && (WheelZooms() || IsZoomModifierHeld())) {
        const UIRectF strip = Strip();
        const float pivotX = HitTestRect(strip, m_Cursor) ? m_Cursor.x : strip.Min().x + strip.Size.x * 0.5f;
        Owner->ZoomAt(strip, pivotX, InDelta.y);
        return true;
    }
    return false;
}

void AnimationTimelineStrip::PaintGrid(UIDrawList& OutDrawList, const Vec4& InColor) {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!animation) {
        return;
    }
    const UIRectF strip = Strip();
    const int32_t step = Owner->LabelledFrameStep(strip);
    const int32_t first = ((int32_t)std::floor(Owner->GetViewStart()) / step) * step;

    for (int32_t frame = first; frame <= (int32_t)std::ceil(Owner->GetViewEnd()); frame += step) {
        if (frame < 0) {
            continue;
        }
        const float x = Owner->FrameToX(strip, (float)frame);
        OutDrawList.AddRect(UIRectF(Vec2(x, m_Geometry.Min().y), Vec2(1.0f, m_Geometry.Size.y)), InColor, m_WorldMatrix);
    }
}

void AnimationTimelineStrip::PaintOutOfRange(UIDrawList& OutDrawList) {
    if (!Owner) {
        return;
    }
    const UIRectF strip = Strip();
    const float start = Owner->GetOutOfRangeStartX(strip);
    const float end = Owner->GetOutOfRangeEndX(strip);

    if (start > strip.Min().x) {
        const float width = (start < strip.Max().x ? start : strip.Max().x) - strip.Min().x;
        OutDrawList.AddRect(UIRectF(Vec2(strip.Min().x, m_Geometry.Min().y), Vec2(width, m_Geometry.Size.y)),
                            s_OutOfRange, m_WorldMatrix);
    }
    if (end < strip.Max().x) {
        const float left = end > strip.Min().x ? end : strip.Min().x;
        OutDrawList.AddRect(UIRectF(Vec2(left, m_Geometry.Min().y), Vec2(strip.Max().x - left, m_Geometry.Size.y)),
                            s_OutOfRange, m_WorldMatrix);
    }
}

void AnimationTimelineStrip::PaintPlayHead(UIDrawList& OutDrawList) {
    AnimationEditorTab* editor = Owner ? Owner->GetEditor() : nullptr;
    if (!editor) {
        return;
    }
    const UIRectF strip = Strip();
    const float x = Owner->FrameToX(strip, editor->GetFramePosition());
    if (x < strip.Min().x - 1.0f || x > strip.Max().x + 1.0f) {
        return;
    }
    OutDrawList.AddRect(UIRectF(Vec2(x - 0.5f, m_Geometry.Min().y), Vec2(1.0f, m_Geometry.Size.y)), s_PlayHead, m_WorldMatrix);
}

/* -------------------------------- Ruler -------------------------------- */

AnimationTimelineRuler::AnimationTimelineRuler() {
    Cursor = CursorIcon::ResizeH;
    ClipChildren = true;
}

void AnimationTimelineRuler::Paint(UIDrawList& OutDrawList) {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    OutDrawList.AddRect(m_Geometry, EditorStyle::TabBar, m_WorldMatrix);
    if (!animation) {
        return;
    }

    PaintOutOfRange(OutDrawList);

    const UIRectF strip = Strip();
    const int32_t step = Owner->LabelledFrameStep(strip);
    const int32_t first = ((int32_t)std::floor(Owner->GetViewStart()) / step) * step;
    Font* font = UINode::GetDefaultFont();

    for (int32_t frame = first; frame <= (int32_t)std::ceil(Owner->GetViewEnd()); frame += step) {
        if (frame < 0) {
            continue;
        }
        const float x = Owner->FrameToX(strip, (float)frame);
        const Vec4 tint = frame > animation->GetFrameCount() ? s_OutOfRangeText : EditorStyle::TextDim;
        OutDrawList.AddRect(UIRectF(Vec2(x, m_Geometry.Max().y - 6.0f), Vec2(1.0f, 6.0f)), tint, m_WorldMatrix);
        if (font) {
            OutDrawList.AddText(font, std::to_string(frame), Vec2(x + 3.0f, m_Geometry.Min().y + 3.0f),
                                EditorStyle::FontSize - 3.0f, tint, m_WorldMatrix);
        }
    }

    PaintPlayHead(OutDrawList);
}

void AnimationTimelineRuler::OnPressed(const Vec2& InCursorPos) {
    if (Owner && !IsOverLabelColumn(InCursorPos)) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
    }
}

void AnimationTimelineRuler::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (Owner) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
    }
}

/* -------------------------------- Events -------------------------------- */

AnimationTimelineEvents::AnimationTimelineEvents() {
    Cursor = CursorIcon::ResizeH;
    ClipChildren = true;

    UILabel* caption = Add<UILabel>();
    caption->Anchor = caption->Pivot = Vec2(0.0f, 0.5f);
    caption->Position = Vec2(10.0f, 0.0f);
    caption->Size = Vec2(52.0f, AnimationTimelineTab::EventsHeight);
    caption->FontSize = EditorStyle::FontSize - 1.0f;
    caption->Color = EditorStyle::TextDim;
    caption->VAlign = UIVAlign::Middle;
    caption->Text = "Events";

    m_Name = Add<UITextArea>();
    m_Name->Anchor = m_Name->Pivot = Vec2(0.0f, 0.5f);
    m_Name->Position = Vec2(64.0f, 0.0f);
    m_Name->SingleLine = true;
    m_Name->FontSize = EditorStyle::FontSize - 1.0f;
    m_Name->TextColor = EditorStyle::TextBright;
    m_Name->CaretColor = EditorStyle::TextBright;
    m_Name->BackgroundColor = EditorStyle::PanelDark;
    m_Name->FocusedBorderColor = EditorStyle::Accent;
    m_Name->Padding = UIPadding(5.0f, 0.0f);
    m_Name->Placeholder = "Event name";
    m_Name->TextChanged = [this](const String& InText) {
        Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
        if (animation) {
            animation->SetEventName(Owner->GetSelectedEvent(), InText);
        }
    };

    m_Add = Add<UIButton>();
    m_Add->Anchor = m_Add->Pivot = Vec2(0.0f, 0.5f);
    m_Add->Size = Vec2(20.0f, 18.0f);
    EditorStyle::StyleButton(*m_Add);
    UISvg* addIcon = m_Add->Add<UISvg>();
    addIcon->Center(Vec2(10.0f, 10.0f));
    addIcon->Image = EditorIcons::Add();
    addIcon->Tint = EditorStyle::Text;
    m_Add->Add<UITooltip>()->Text = "Add an event at the playhead";
    m_Add->Clicked = [this] {
        AnimationEditorTab* editor = Owner ? Owner->GetEditor() : nullptr;
        if (editor) {
            Owner->AddEventAt(editor->GetFrame());
        }
    };

    Bind = [this] {
        const float labelWidth = Owner ? Owner->GetLabelWidth() : AnimationTimelineTab::DefaultLabelWidth;
        m_Name->Size = Vec2(labelWidth - 100.0f, AnimationTimelineTab::EventsHeight - 4.0f);
        m_Add->Position = Vec2(labelWidth - 30.0f, 0.0f);

        Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
        const int32_t selected = Owner ? Owner->GetSelectedEvent() : -1;
        const bool editing = animation && selected >= 0 && selected < animation->GetEvents().Size();
        m_Name->SetEnabled(editing);
        if (editing && !m_Name->IsFocused()) {
            m_Name->Text = animation->GetEvents()[selected].Name;
        }
    };
}

void AnimationTimelineEvents::BeginRename() {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    const int32_t selected = Owner ? Owner->GetSelectedEvent() : -1;
    if (!animation || selected < 0 || selected >= animation->GetEvents().Size()) {
        return;
    }
    m_Name->Text = animation->GetEvents()[selected].Name;
    m_Name->SetEnabled(true);
    m_Name->RequestFocus();
    m_Name->SelectAll();
}

int32_t AnimationTimelineEvents::EventAt(const Vec2& InCursorPos) const {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!animation) {
        return -1;
    }
    const UIRectF strip = Strip();
    for (int32_t i = animation->GetEvents().Size() - 1; i >= 0; i--) {
        if (std::abs(Owner->FrameToX(strip, (float)animation->GetEvents()[i].Frame) - InCursorPos.x) <= s_KeyRadius + 1.0f) {
            return i;
        }
    }
    return -1;
}

int32_t AnimationTimelineEvents::FrameAt(const Vec2& InCursorPos) const {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    const int32_t frame = (int32_t)std::lround(Owner->XToFrame(Strip(), InCursorPos.x));
    return std::clamp(frame, 0, animation ? animation->GetFrameCount() : 0);
}

void AnimationTimelineEvents::Paint(UIDrawList& OutDrawList) {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    OutDrawList.AddRect(m_Geometry, EditorStyle::TabBar, m_WorldMatrix);
    if (!animation) {
        return;
    }

    PaintOutOfRange(OutDrawList);
    PaintGrid(OutDrawList, s_Grid);
    PaintPlayHead(OutDrawList);

    const UIRectF strip = Strip();
    Font* font = UINode::GetDefaultFont();
    const float top = m_Geometry.Min().y + 3.0f;
    const float height = m_Geometry.Size.y - 6.0f;
    for (int32_t i = 0; i < animation->GetEvents().Size(); i++) {
        const AnimationEvent& event = animation->GetEvents()[i];
        const float x = Owner->FrameToX(strip, (float)event.Frame);
        if (x < strip.Min().x || x > strip.Max().x) {
            continue;
        }
        const bool selected = i == Owner->GetSelectedEvent();
        const Vec4 color = selected ? EditorStyle::AccentBright : s_EventColor;
        OutDrawList.AddRoundedRect(UIRectF(Vec2(x - 3.0f, top), Vec2(6.0f, height)), color, 2.0f, m_WorldMatrix);
        if (font && !event.Name.empty()) {
            OutDrawList.AddText(font, event.Name, Vec2(x + 6.0f, top + 1.0f), EditorStyle::FontSize - 3.0f,
                                selected ? EditorStyle::TextBright : EditorStyle::TextDim, m_WorldMatrix);
        }
    }
}

void AnimationTimelineEvents::OnPressed(const Vec2& InCursorPos) {
    m_DraggedEvent = -1;
    if (!Owner || IsOverLabelColumn(InCursorPos)) {
        return;
    }
    m_DraggedEvent = EventAt(InCursorPos);
    Owner->SelectEvent(m_DraggedEvent);
    if (m_DraggedEvent < 0) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
    }
}

void AnimationTimelineEvents::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!animation) {
        return;
    }
    if (m_DraggedEvent < 0) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
        return;
    }
    animation->SetEventFrame(m_DraggedEvent, FrameAt(InCursorPos));
}

void AnimationTimelineEvents::OnReleased(bool InInside) {
    (void)InInside;
    m_DraggedEvent = -1;
}

bool AnimationTimelineEvents::OnSecondaryClick(const Vec2& InCursorPos) {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!animation || IsOverLabelColumn(InCursorPos)) {
        return false;
    }

    const WeakObjectPtr<AnimationTimelineEvents> self = this;
    const int32_t hovered = EventAt(InCursorPos);
    UIMenuModel menu;
    if (hovered >= 0) {
        Owner->SelectEvent(hovered);
        menu.Section(animation->GetEvents()[hovered].Name);
        menu.Item("Rename", [self] {
            if (self.Get()) {
                self.Get()->BeginRename();
            }
        });
        menu.Item("Delete Event", [self] {
            if (self.Get() && self.Get()->Owner) {
                self.Get()->Owner->DeleteSelectedEvent();
            }
        }).Icon(EditorIcons::Delete()).Shortcut("Del");
    } else {
        const int32_t frame = FrameAt(InCursorPos);
        menu.Item("Add Event Here", [self, frame] {
            if (self.Get() && self.Get()->Owner) {
                self.Get()->Owner->AddEventAt(frame);
            }
        }).Icon(EditorIcons::Add()).Enabled(animation->FindEvent(frame) < 0);
    }
    UIContextMenu::OpenAt(*this, InCursorPos, menu);
    return true;
}

/* -------------------------------- Summary -------------------------------- */

AnimationTimelineSummary::AnimationTimelineSummary() {
    Cursor = CursorIcon::ResizeH;
    ClipChildren = true;
}

int32_t AnimationTimelineSummary::ColumnAt(const Vec2& InCursorPos) const {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!animation || IsOverLabelColumn(InCursorPos)) {
        return -1;
    }
    const UIRectF strip = Strip();
    for (const AnimationKey& key : animation->GetKeys()) {
        if (std::abs(Owner->FrameToX(strip, (float)key.Frame) - InCursorPos.x) <= s_KeyRadius + 1.0f) {
            return key.Frame;
        }
    }
    return -1;
}

void AnimationTimelineSummary::Paint(UIDrawList& OutDrawList) {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    OutDrawList.AddRect(m_Geometry, EditorStyle::PanelDark, m_WorldMatrix);
    if (!animation) {
        return;
    }

    PaintOutOfRange(OutDrawList);
    PaintGrid(OutDrawList, s_Grid);
    PaintPlayHead(OutDrawList);

    const UIRectF strip = Strip();
    const float centerY = m_Geometry.Min().y + m_Geometry.Size.y * 0.5f;

    // One diamond per frame that holds any key, however many tracks share it.
    Array<int32_t> painted;
    for (const AnimationKey& key : animation->GetKeys()) {
        if (painted.Contains(key.Frame)) {
            continue;
        }
        painted.Add(key.Frame);
        const float x = Owner->FrameToX(strip, (float)key.Frame);
        if (x < strip.Min().x || x > strip.Max().x) {
            continue;
        }
        if (key.Frame == m_DraggedFrame) {
            PaintDiamond(OutDrawList, Vec2(x, centerY), s_KeyRadius + 1.0f, EditorStyle::AccentBright, m_WorldMatrix);
        }
        PaintDiamond(OutDrawList, Vec2(x, centerY), s_KeyRadius - 1.0f, s_KeyColor, m_WorldMatrix);
    }
}

void AnimationTimelineSummary::OnPressed(const Vec2& InCursorPos) {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    m_DraggedKeys.Clear();
    m_DraggedFrame = ColumnAt(InCursorPos);
    if (!animation || IsOverLabelColumn(InCursorPos)) {
        return;
    }
    if (m_DraggedFrame < 0) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
        return;
    }
    for (int32_t i = 0; i < animation->GetKeys().Size(); i++) {
        if (animation->GetKeys()[i].Frame == m_DraggedFrame) {
            m_DraggedKeys.Add(i);
        }
    }
    Owner->SelectKey(-1);
    Owner->SelectEvent(-1);
}

void AnimationTimelineSummary::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!animation) {
        return;
    }
    if (m_DraggedKeys.IsEmpty()) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
        return;
    }
    const int32_t frame = (int32_t)std::lround(Owner->XToFrame(Strip(), InCursorPos.x));
    m_DraggedFrame = std::clamp(frame, 0, animation->GetFrameCount());
    animation->MoveKeys(m_DraggedKeys, m_DraggedFrame);
}

void AnimationTimelineSummary::OnReleased(bool InInside) {
    (void)InInside;
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (animation && !m_DraggedKeys.IsEmpty()) {
        animation->MergeKeys(m_DraggedKeys);
    }
    m_DraggedKeys.Clear();
    m_DraggedFrame = -1;
}

/* -------------------------------- Row -------------------------------- */

AnimationTimelineRow::AnimationTimelineRow() {
    ClipChildren = true;

    m_Expander = Add<UISvg>();
    m_Expander->Anchor = m_Expander->Pivot = Vec2(0.0f, 0.5f);
    m_Expander->Size = Vec2(11.0f, 11.0f);
    m_Expander->Tint = EditorStyle::TextDim;

    m_Icon = Add<UISvg>();
    m_Icon->Anchor = m_Icon->Pivot = Vec2(0.0f, 0.5f);
    m_Icon->Size = Vec2(14.0f, 14.0f);
    m_Icon->Tint = EditorStyle::Text;

    m_Label = Add<UILabel>();
    m_Label->Anchor = m_Label->Pivot = Vec2(0.0f, 0.5f);
    m_Label->FontSize = EditorStyle::FontSize - 1.0f;
    m_Label->Color = EditorStyle::Text;
    m_Label->VAlign = UIVAlign::Middle;

    m_ValueHost = Add<UINode>();
    m_ValueHost->Anchor = m_ValueHost->Pivot = Vec2(0.0f, 0.5f);
    m_ValueHost->ClipChildren = true;

    Bind = [this] { Refresh(); };
}

void AnimationTimelineRow::SetEditor(Node* InTarget, Property* InLeaf) {
    if (InTarget == m_EditorTarget.Get() && InLeaf == m_EditorLeaf) {
        return;
    }
    m_EditorTarget = InTarget;
    m_EditorLeaf = InLeaf;

    while (m_ValueHost->GetChildCount() > 0) {
        delete m_ValueHost->GetChild(0);
    }
    if (InLeaf) {
        Owner->BuildRowEditor(*m_ValueHost, RowIndex);
    }
}

void AnimationTimelineRow::Refresh() {
    const TimelineRow* row = Owner ? Owner->GetRow(RowIndex) : nullptr;
    const AnimationTrack* track = Owner ? Owner->GetRowTrack(RowIndex) : nullptr;
    if (!row || !track) {
        m_Expander->SetEnabled(false);
        m_Icon->SetEnabled(false);
        m_Label->Text.clear();
        SetEditor(nullptr, nullptr);
        return;
    }

    const float indent = 6.0f + (float)row->Depth * AnimationTimelineTab::IndentStep;
    m_Expander->SetEnabled(row->Expandable);
    m_Expander->Position = Vec2(indent, 0.0f);
    m_Expander->Image = row->Expanded ? EditorIcons::ArrowDown() : EditorIcons::ArrowRight();

    VectorImage* icon = Owner->GetRowIcon(RowIndex);
    m_Icon->SetEnabled(icon != nullptr);
    m_Icon->Image = icon;
    m_Icon->Position = Vec2(indent + 13.0f, 0.0f);

    const TimelineValueRef ref = Owner->ResolveRow(RowIndex);
    const float labelWidth = Owner->GetLabelWidth();
    const float textLeft = indent + 30.0f;
    const float available = labelWidth - textLeft - 16.0f;
    const int32_t components = DetailsCustomization::GetInnerProperties(ref.Leaf).Size();
    const float wanted = components > 1 ? (float)components * AnimationTimelineTab::ComponentWidth : AnimationTimelineTab::ValueColumnWidth;
    const float valueWidth = std::min(std::max(wanted, available * 0.45f), std::max(available * 0.7f, AnimationTimelineTab::ValueColumnWidth));

    m_Label->Position = Vec2(textLeft, 0.0f);
    m_Label->Size = { available - valueWidth, AnimationTimelineTab::RowHeight };
    m_Label->Text = row->Label;
    m_Label->Color = Owner->GetTrackNode(*track) ? EditorStyle::Text : EditorStyle::TextDim;

    m_ValueHost->Position = Vec2(labelWidth - valueWidth - 6.0f, 0.0f);
    m_ValueHost->Size = Vec2(valueWidth, AnimationTimelineTab::RowHeight - 5.0f);
    SetEditor(ref.Target, ref.Leaf);
}

int32_t AnimationTimelineRow::KeyAt(const Vec2& InCursorPos) const {
    const AnimationTrack* track = Owner ? Owner->GetRowTrack(RowIndex) : nullptr;
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!track || !animation) {
        return -1;
    }

    const UIRectF strip = Strip();
    for (int32_t key : track->Keys) {
        if (std::abs(Owner->FrameToX(strip, (float)animation->GetKeys()[key].Frame) - InCursorPos.x) <= s_KeyRadius + 2.0f) {
            return key;
        }
    }
    return -1;
}

void AnimationTimelineRow::Paint(UIDrawList& OutDrawList) {
    const AnimationTrack* track = Owner ? Owner->GetRowTrack(RowIndex) : nullptr;
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!track || !animation) {
        return;
    }

    const bool selected = track->Keys.Contains(Owner->GetSelectedKey());
    OutDrawList.AddRect(m_Geometry, selected ? s_RowSelected : ((RowIndex & 1) ? s_RowOdd : s_RowEven), m_WorldMatrix);

    PaintOutOfRange(OutDrawList);
    PaintGrid(OutDrawList, s_Grid);
    PaintPlayHead(OutDrawList);

    const UIRectF strip = Strip();
    const float centerY = m_Geometry.Min().y + m_Geometry.Size.y * 0.5f;
    for (int32_t key : track->Keys) {
        const AnimationKey& frameKey = animation->GetKeys()[key];
        const float x = Owner->FrameToX(strip, (float)frameKey.Frame);
        if (x < strip.Min().x || x > strip.Max().x) {
            continue;
        }
        if (key == Owner->GetSelectedKey()) {
            PaintDiamond(OutDrawList, Vec2(x, centerY), s_KeyRadius + 2.0f, EditorStyle::AccentBright, m_WorldMatrix);
        }
        PaintDiamond(OutDrawList, Vec2(x, centerY), s_KeyRadius,
                     frameKey.Interpolation == AnimationInterpolation::None ? s_KeyStepColor : s_KeyColor, m_WorldMatrix);
    }
}

void AnimationTimelineRow::OnPressed(const Vec2& InCursorPos) {
    const AnimationTrack* track = Owner ? Owner->GetRowTrack(RowIndex) : nullptr;
    m_DraggedKey = -1;
    m_PressedOnLabel = IsOverLabelColumn(InCursorPos);
    if (!track) {
        return;
    }

    if (m_PressedOnLabel) {
        if (m_Expander->IsEnabled() && m_Expander->HitTest(InCursorPos)) {
            Owner->ToggleExpanded(RowIndex);
            return;
        }
        if (Node* node = Owner->GetTrackNode(*track)) {
            if (MajorTab* major = Owner->GetMajorTab()) {
                major->SetSelection(node);
            }
        }
        return;
    }

    m_DraggedKey = KeyAt(InCursorPos);
    Owner->SelectKey(m_DraggedKey);
    if (m_DraggedKey < 0) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
    }
}

void AnimationTimelineRow::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!animation || m_PressedOnLabel) {
        return;
    }

    if (m_DraggedKey < 0) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
        return;
    }

    animation->MoveKeys({ m_DraggedKey }, (int32_t)std::lround(Owner->XToFrame(Strip(), InCursorPos.x)));
}

void AnimationTimelineRow::OnReleased(bool InInside) {
    (void)InInside;
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (animation && m_DraggedKey >= 0) {
        Owner->SelectKey(animation->MergeKeys({ m_DraggedKey })[0]);
    }
    m_DraggedKey = -1;
    m_PressedOnLabel = false;
}

bool AnimationTimelineRow::OnSecondaryClick(const Vec2& InCursorPos) {
    const AnimationTrack* track = Owner ? Owner->GetRowTrack(RowIndex) : nullptr;
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    if (!track || !animation || IsOverLabelColumn(InCursorPos)) {
        return false;
    }

    // Menu entries outlive the key list, so they re-find their key by identity, never by index.
    const Array<int32_t> path = track->Path;
    const String propertyName = track->PropertyName;
    const int32_t hovered = KeyAt(InCursorPos);
    const int32_t keyFrame = hovered >= 0 ? animation->GetKeys()[hovered].Frame : 0;
    const WeakObjectPtr<AnimationTimelineTab> owner = Owner;

    const auto findKey = [owner, path, propertyName, keyFrame]() -> int32_t {
        Animation* open = owner.Get() ? owner.Get()->GetAnimation() : nullptr;
        return open ? open->FindKey(path, propertyName, keyFrame) : -1;
    };

    UIMenuModel menu;
    menu.Section(Owner->GetTrackLabel(*track));

    if (hovered >= 0) {
        const AnimationInterpolation current = animation->GetKeys()[hovered].Interpolation;
        const auto mode = [&menu, owner, findKey, current](const String& InLabel, AnimationInterpolation InMode) {
            menu.Item(InLabel, [owner, findKey, InMode] {
                Animation* open = owner.Get() ? owner.Get()->GetAnimation() : nullptr;
                if (open) {
                    open->SetKeyInterpolation(findKey(), InMode);
                }
            }).Checked(current == InMode);
        };
        menu.Section("Interpolation");
        mode("None", AnimationInterpolation::None);
        mode("Linear", AnimationInterpolation::Linear);
        mode("Ease In", AnimationInterpolation::EaseIn);
        mode("Ease Out", AnimationInterpolation::EaseOut);
        mode("Ease In Out", AnimationInterpolation::EaseInOut);
        menu.Separator();
        menu.Item("Add Event Here", [owner, keyFrame] {
            if (AnimationTimelineTab* timeline = owner.Get()) {
                timeline->AddEventAt(keyFrame);
            }
        }).Icon(EditorIcons::Add()).Enabled(animation->FindEvent(keyFrame) < 0);
        menu.Item("Delete Key", [owner, findKey] {
            Animation* open = owner.Get() ? owner.Get()->GetAnimation() : nullptr;
            if (open) {
                open->RemoveKey(findKey());
                owner.Get()->SelectKey(-1);
            }
        }).Shortcut("Del");
    } else {
        const float raw = Owner->XToFrame(Strip(), InCursorPos.x);
        const int32_t frame = (int32_t)std::lround(raw < 0.0f ? 0.0f : raw);
        menu.Item("Add Key Here", [owner, path, propertyName, frame] {
            AnimationTimelineTab* timeline = owner.Get();
            AnimationEditorTab* editor = timeline ? timeline->GetEditor() : nullptr;
            Animation* open = timeline ? timeline->GetAnimation() : nullptr;
            Node* root = editor ? editor->GetAnimationRoot() : nullptr;
            Node* node = root ? Animation::ResolvePath(*root, path) : nullptr;
            if (open && root && node) {
                open->KeyProperty(*root, *node, propertyName, frame);
            }
        }).Tooltip("Records what the node currently holds at this frame");
    }

    menu.Separator();
    menu.Item("Remove Property", [owner, path, propertyName] {
        if (AnimationTimelineTab* timeline = owner.Get()) {
            if (Animation* open = timeline->GetAnimation()) {
                open->RemoveTrack(path, propertyName);
                timeline->SelectKey(-1);
            }
        }
    }).Tooltip("Deletes every key of this property");

    UIContextMenu::OpenAt(*this, InCursorPos, menu);
    return true;
}

/* -------------------------------- Splitter -------------------------------- */

AnimationTimelineSplitter::AnimationTimelineSplitter() {
    Interactable = true;
    Cursor = CursorIcon::ResizeH;
    Anchor = Pivot = Vec2(0.0f);
    Size = { 7.0_px, 1.0_rel - UIValue::Px(AnimationTimelineTab::TransportHeight) };
    Bind = [this] {
        Position = Vec2((Owner ? Owner->GetLabelWidth() : AnimationTimelineTab::DefaultLabelWidth) - 3.0f,
                        AnimationTimelineTab::TransportHeight);
    };
}

void AnimationTimelineSplitter::Paint(UIDrawList& OutDrawList) {
    const bool active = IsHovered() || IsPressed();
    OutDrawList.AddRect(UIRectF(Vec2(m_Geometry.Min().x + 3.0f, m_Geometry.Min().y), Vec2(1.0f, m_Geometry.Size.y)),
                        active ? EditorStyle::Accent : EditorStyle::Border, m_WorldMatrix);
}

void AnimationTimelineSplitter::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (Owner) {
        Owner->SetLabelWidth(InCursorPos.x - Owner->GetGeometry().Min().x);
    }
}

/* -------------------------------- Scroll bar -------------------------------- */

AnimationTimelineScrollBar::AnimationTimelineScrollBar() {
    Interactable = true;
    Cursor = CursorIcon::ResizeH;
}

UIRectF AnimationTimelineScrollBar::ThumbRect() const {
    const UIRectF track = Owner->StripOf(m_Geometry);
    float domainStart = 0.0f;
    float domainEnd = 1.0f;
    Owner->GetScrollDomain(domainStart, domainEnd);

    // Zoomed out past the domain the view reaches beyond both ends, and the thumb fills the track.
    const auto share = [domainStart, domainEnd](float InFrame) {
        return std::clamp((InFrame - domainStart) / (domainEnd - domainStart), 0.0f, 1.0f);
    };
    const float left = share(Owner->GetViewStart()) * track.Size.x;
    const float width = std::max(share(Owner->GetViewEnd()) * track.Size.x - left, s_MinThumbWidth);
    return UIRectF(Vec2(track.Min().x + std::max(std::min(left, track.Size.x - width), 0.0f), track.Min().y + 2.0f),
                   Vec2(width, track.Size.y - 4.0f));
}

void AnimationTimelineScrollBar::Paint(UIDrawList& OutDrawList) {
    if (!Owner) {
        return;
    }
    OutDrawList.AddRect(m_Geometry, EditorStyle::BottomBar, m_WorldMatrix);

    const UIRectF track = Owner->StripOf(m_Geometry);
    OutDrawList.AddRoundedRect(UIRectF(Vec2(track.Min().x, track.Min().y + 3.0f), Vec2(track.Size.x, track.Size.y - 6.0f)),
                               s_ScrollTrack, 2.0f, m_WorldMatrix);
    OutDrawList.AddRoundedRect(ThumbRect(), IsHovered() ? s_ScrollThumbHover : s_ScrollThumb, 3.0f, m_WorldMatrix);
}

void AnimationTimelineScrollBar::OnPressed(const Vec2& InCursorPos) {
    const UIRectF thumb = ThumbRect();
    // Grabbing the thumb keeps it under the cursor; a click beside it centres the view there.
    m_GrabOffset = HitTestRect(thumb, InCursorPos) ? InCursorPos.x - thumb.Min().x : thumb.Size.x * 0.5f;
    OnDrag(InCursorPos, Vec2(0.0f));
}

void AnimationTimelineScrollBar::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (!Owner) {
        return;
    }
    const UIRectF track = Owner->StripOf(m_Geometry);
    float domainStart = 0.0f;
    float domainEnd = 1.0f;
    Owner->GetScrollDomain(domainStart, domainEnd);

    // The thumb travels the track minus its own width, and the view the domain minus what it shows.
    const float travel = track.Size.x - ThumbRect().Size.x;
    const float alpha = travel > 0.0f ? (InCursorPos.x - m_GrabOffset - track.Min().x) / travel : 0.0f;
    Owner->SetViewStart(domainStart + alpha * (domainEnd - domainStart - (Owner->GetViewEnd() - Owner->GetViewStart())));
}
