#include "AnimationTimelineWidgets.h"
#include "AnimationTimelineTab.h"
#include "AnimationEditorTab.h"
#include "MajorTab.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIContextMenu.h"
#include "UI/UIMenuModel.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "UI/UIDragNumber.h"
#include "GameFramework/Node.h"
#include "Assets/Animation.h"
#include "Assets/Font.h"
#include "Rendering/UIDrawList.h"
#include "InputSystem/KeyboardDevice.h"
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

static constexpr float s_KeyRadius = 5.0f;

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
    return AnimationTimelineTab::StripOf(m_Geometry);
}

bool AnimationTimelineStrip::IsOverLabelColumn(const Vec2& InCursorPos) const {
    return InCursorPos.x < m_Geometry.Min().x + AnimationTimelineTab::LabelColumnWidth;
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

/* -------------------------------- Summary -------------------------------- */

AnimationTimelineSummary::AnimationTimelineSummary() {
    Cursor = CursorIcon::ResizeH;
    ClipChildren = true;
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
        if (x >= strip.Min().x && x <= strip.Max().x) {
            PaintDiamond(OutDrawList, Vec2(x, centerY), s_KeyRadius - 1.0f, s_KeyColor, m_WorldMatrix);
        }
    }
}

void AnimationTimelineSummary::OnPressed(const Vec2& InCursorPos) {
    if (Owner && !IsOverLabelColumn(InCursorPos)) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
    }
}

void AnimationTimelineSummary::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (Owner) {
        Owner->ScrubTo(Strip(), InCursorPos.x);
    }
}

/* -------------------------------- Row -------------------------------- */

AnimationTimelineRow::AnimationTimelineRow() {
    ClipChildren = true;

    m_Expander = Add<UISvg>();
    m_Expander->Anchor = m_Expander->Pivot = Vec2(0.0f, 0.5f);
    m_Expander->Size = Vec2(11.0f, 11.0f);
    m_Expander->Tint = EditorStyle::TextDim;

    m_Label = Add<UILabel>();
    m_Label->Anchor = m_Label->Pivot = Vec2(0.0f, 0.5f);
    m_Label->FontSize = EditorStyle::FontSize - 1.0f;
    m_Label->Color = EditorStyle::Text;
    m_Label->VAlign = UIVAlign::Middle;

    m_ValueLabel = Add<UILabel>();
    m_ValueLabel->Anchor = m_ValueLabel->Pivot = Vec2(0.0f, 0.5f);
    m_ValueLabel->Size = { AnimationTimelineTab::ValueColumnWidth, AnimationTimelineTab::RowHeight };
    m_ValueLabel->FontSize = EditorStyle::FontSize - 2.0f;
    m_ValueLabel->Color = EditorStyle::TextDim;
    m_ValueLabel->VAlign = UIVAlign::Middle;

    m_ValueField = Add<UIDragNumber>();
    m_ValueField->Anchor = m_ValueField->Pivot = Vec2(0.0f, 0.5f);
    m_ValueField->Size = Vec2(AnimationTimelineTab::ValueColumnWidth, AnimationTimelineTab::RowHeight - 4.0f);
    m_ValueField->Decimals = 3;
    m_ValueField->Sensitivity = 0.02;
    m_ValueField->Get = [this] {
        return Owner ? Owner->ReadRowValue(Owner->ResolveRow(RowIndex)) : 0.0;
    };
    m_ValueField->Set = [this](double InValue) {
        if (Owner) {
            Owner->WriteRowValue(Owner->ResolveRow(RowIndex), InValue);
        }
    };

    Bind = [this] { Refresh(); };
}

void AnimationTimelineRow::Refresh() {
    const TimelineRow* row = Owner ? Owner->GetRow(RowIndex) : nullptr;
    const AnimationTrack* track = Owner ? Owner->GetRowTrack(RowIndex) : nullptr;
    if (!row || !track) {
        m_Expander->SetEnabled(false);
        m_Label->Text.clear();
        m_ValueLabel->SetEnabled(false);
        m_ValueField->SetEnabled(false);
        return;
    }

    const float indent = 8.0f + (row->Component >= 0 ? AnimationTimelineTab::IndentStep : 0.0f);
    m_Expander->SetEnabled(row->Expandable);
    m_Expander->Position = Vec2(indent, 0.0f);
    m_Expander->Image = row->Expanded ? EditorIcons::ArrowDown() : EditorIcons::ArrowRight();

    const float textLeft = indent + (row->Expandable ? 15.0f : 2.0f);
    m_Label->Position = Vec2(textLeft, 0.0f);
    m_Label->Size = { AnimationTimelineTab::LabelColumnWidth - AnimationTimelineTab::ValueColumnWidth - textLeft - 6.0f,
                      AnimationTimelineTab::RowHeight };
    m_Label->Text = row->Label;
    m_Label->Color = Owner->GetTrackNode(*track) ? EditorStyle::Text : EditorStyle::TextDim;

    const float valueLeft = AnimationTimelineTab::LabelColumnWidth - AnimationTimelineTab::ValueColumnWidth - 4.0f;
    const bool numeric = Owner->ResolveRow(RowIndex).Numeric != nullptr;
    m_ValueField->SetEnabled(numeric);
    m_ValueField->Position = Vec2(valueLeft, 0.0f);

    const String text = numeric ? String() : Owner->RowValueText(RowIndex);
    m_ValueLabel->SetEnabled(!text.empty());
    m_ValueLabel->Position = Vec2(valueLeft + 2.0f, 0.0f);
    m_ValueLabel->Text = text;
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

    const float frame = Owner->XToFrame(Strip(), InCursorPos.x);
    const int32_t clamped = (int32_t)std::lround(frame < 0.0f ? 0.0f : frame);
    m_DraggedKey = animation->SetKeyFrame(m_DraggedKey, clamped > animation->GetFrameCount() ? animation->GetFrameCount() : clamped);
    Owner->SelectKey(m_DraggedKey);
}

void AnimationTimelineRow::OnReleased(bool InInside) {
    (void)InInside;
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

/* -------------------------------- Scroll bar -------------------------------- */

AnimationTimelineScrollBar::AnimationTimelineScrollBar() {
    Interactable = true;
    Cursor = CursorIcon::ResizeH;
}

void AnimationTimelineScrollBar::Domain(float& OutStart, float& OutSpan) const {
    Animation* animation = Owner ? Owner->GetAnimation() : nullptr;
    const float total = animation ? (float)animation->GetFrameCount() : 1.0f;
    // The bar covers the animation plus however far the view has been pushed outside it.
    OutStart = Owner->GetViewStart() < 0.0f ? Owner->GetViewStart() : 0.0f;
    const float end = Owner->GetViewEnd() > total ? Owner->GetViewEnd() : total;
    OutSpan = end - OutStart;
    if (OutSpan <= 0.0f) {
        OutSpan = 1.0f;
    }
}

UIRectF AnimationTimelineScrollBar::ThumbRect() const {
    const UIRectF track = AnimationTimelineTab::StripOf(m_Geometry);
    float domainStart = 0.0f;
    float domainSpan = 1.0f;
    Domain(domainStart, domainSpan);

    const float start = (Owner->GetViewStart() - domainStart) / domainSpan;
    const float end = (Owner->GetViewEnd() - domainStart) / domainSpan;
    const float width = (end - start) * track.Size.x;
    return UIRectF(Vec2(track.Min().x + start * track.Size.x, track.Min().y + 2.0f),
                   Vec2(width < 16.0f ? 16.0f : width, track.Size.y - 4.0f));
}

void AnimationTimelineScrollBar::Paint(UIDrawList& OutDrawList) {
    if (!Owner) {
        return;
    }
    OutDrawList.AddRect(m_Geometry, EditorStyle::BottomBar, m_WorldMatrix);

    const UIRectF track = AnimationTimelineTab::StripOf(m_Geometry);
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
    const UIRectF track = AnimationTimelineTab::StripOf(m_Geometry);
    if (!Owner || track.Size.x <= 0.0f) {
        return;
    }
    float domainStart = 0.0f;
    float domainSpan = 1.0f;
    Domain(domainStart, domainSpan);
    Owner->SetViewStart(domainStart + (InCursorPos.x - m_GrabOffset - track.Min().x) / track.Size.x * domainSpan);
}
