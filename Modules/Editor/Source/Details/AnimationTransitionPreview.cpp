#include "AnimationTransitionPreview.h"
#include "DetailsCustomization.h"
#include "Tabs/DetailsTab.h"
#include "UI/EditorStyle.h"
#include "Assets/Animation.h"
#include "Assets/AnimationGraph.h"
#include "Assets/Font.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

static const Vec4 s_Bar = HexColor(0x3A3F46);
static const Vec4 s_BarIdle = HexColor(0x2A2D31);
static const Vec4 s_Blend = Vec4(0.29f, 0.64f, 1.0f, 0.28f);
static const Vec4 s_BlendEdge = HexColor(0x4AA3FF);

static constexpr float s_Padding = 10.0f;
static constexpr float s_RulerHeight = 16.0f;
static constexpr float s_BarHeight = 20.0f;
static constexpr float s_BarGap = 6.0f;
static constexpr float s_GrabDistance = 5.0f;

static float StateLength(const GraphNode* InNode) {
    const AnimationStateNode* state = Cast<AnimationStateNode>(InNode);
    Animation* clip = state ? state->Motion.Get() : nullptr;
    const float speed = state ? std::abs(state->Speed) : 0.0f;
    return clip && speed > 0.0f && clip->GetDuration() > 0.0f ? clip->GetDuration() / speed : 1.0f;
}

static String FormatSeconds(float InSeconds) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.2gs", InSeconds);
    return buffer;
}

float AnimationTransitionPreview::Layout::ToX(float InSeconds) const {
    return Track.Min().x + (InSeconds - ViewStart) / (ViewEnd - ViewStart) * Track.Size.x;
}

float AnimationTransitionPreview::Layout::SecondsPerPixel() const {
    return (ViewEnd - ViewStart) / std::max(Track.Size.x, 1.0f);
}

AnimationTransitionPreview::AnimationTransitionPreview() {
    Interactable = true;
    Cursor = CursorIcon::ResizeH;
}

float AnimationTransitionPreview::MeasureHeight(const String& InFilter) const {
    return InFilter.empty() ? Height : 0.0f;
}

bool AnimationTransitionPreview::ComputeLayout(Layout& OutLayout) const {
    AnimationTransition* transition = Transition.Get();
    AnimationGraph* graph = Graph.Get();
    AnimationStateMachine* machine = graph ? graph->GetStateMachine() : nullptr;
    if (!transition || !machine) {
        return false;
    }

    GraphNode* source = machine->FindNode(transition->FromNodeId);
    GraphNode* target = machine->FindNode(transition->ToNodeId);
    if (Cast<AnimationExitNode>(target)) {
        target = machine->GetDefaultState();
    }
    OutLayout.SourceName = source ? source->GetTitle() : String("?");
    OutLayout.TargetName = target ? target->GetTitle() : String("?");
    OutLayout.SourceLength = StateLength(source);
    OutLayout.TargetLength = StateLength(target);
    OutLayout.ExitAt = transition->ExitTime * OutLayout.SourceLength;
    OutLayout.BlendEnd = OutLayout.ExitAt + transition->GetDurationSeconds(OutLayout.SourceLength);
    OutLayout.TargetStart = OutLayout.ExitAt - transition->Offset * OutLayout.TargetLength;

    const float start = std::min(0.0f, OutLayout.TargetStart);
    const float end = std::max({ OutLayout.SourceLength, OutLayout.TargetStart + OutLayout.TargetLength, OutLayout.BlendEnd });
    const float margin = (end - start) * 0.04f;
    OutLayout.ViewStart = start - margin;
    OutLayout.ViewEnd = end + margin;

    const Vec2 min = m_Geometry.Min() + Vec2(s_Padding, 6.0f);
    const float width = m_Geometry.Size.x - 2.0f * s_Padding;
    OutLayout.Track = UIRectF(min, Vec2(width, m_Geometry.Size.y - 12.0f));
    const float sourceY = min.y + s_RulerHeight;
    OutLayout.SourceBar = UIRectF(Vec2(min.x, sourceY), Vec2(width, s_BarHeight));
    OutLayout.TargetBar = UIRectF(Vec2(min.x, sourceY + s_BarHeight + s_BarGap), Vec2(width, s_BarHeight));
    return true;
}

void AnimationTransitionPreview::PaintRuler(UIDrawList& OutDrawList, const Layout& InLayout) {
    Font* font = UINode::GetDefaultFont();
    const float span = InLayout.ViewEnd - InLayout.ViewStart;
    float step = 0.05f;
    for (float candidate : { 0.05f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 30.0f }) {
        step = candidate;
        if (span / candidate <= InLayout.Track.Size.x / 60.0f) {
            break;
        }
    }
    for (float seconds = std::ceil(InLayout.ViewStart / step) * step; seconds <= InLayout.ViewEnd; seconds += step) {
        const float x = InLayout.ToX(seconds);
        OutDrawList.AddRect(UIRectF(Vec2(x, InLayout.Track.Min().y + s_RulerHeight - 5.0f), Vec2(1.0f, 5.0f)),
                            EditorStyle::TextDim, m_WorldMatrix);
        OutDrawList.AddRect(UIRectF(Vec2(x, InLayout.SourceBar.Min().y), Vec2(1.0f, InLayout.TargetBar.Max().y - InLayout.SourceBar.Min().y)),
                            Vec4(1.0f, 1.0f, 1.0f, 0.04f), m_WorldMatrix);
        if (font) {
            OutDrawList.AddText(font, FormatSeconds(std::abs(seconds) < 1e-4f ? 0.0f : seconds), Vec2(x + 3.0f, InLayout.Track.Min().y),
                                EditorStyle::FontSize - 4.0f, EditorStyle::TextDim, m_WorldMatrix);
        }
    }
}

void AnimationTransitionPreview::PaintBar(UIDrawList& OutDrawList, const Layout& InLayout, const UIRectF& InRow, float InStart,
                                          float InLength, float InActiveFrom, float InActiveTo, const String& InName) {
    const float left = InLayout.ToX(InStart);
    const UIRectF bar(Vec2(left, InRow.Min().y), Vec2(InLayout.ToX(InStart + InLength) - left, InRow.Size.y));
    OutDrawList.AddRoundedRect(bar, s_BarIdle, 3.0f, m_WorldMatrix);

    const float activeLeft = std::max(InLayout.ToX(InActiveFrom), bar.Min().x);
    const float activeRight = std::min(InLayout.ToX(InActiveTo), bar.Max().x);
    if (activeRight > activeLeft) {
        OutDrawList.AddRoundedRect(UIRectF(Vec2(activeLeft, bar.Min().y), Vec2(activeRight - activeLeft, bar.Size.y)), s_Bar, 3.0f, m_WorldMatrix);
    }

    if (Font* font = UINode::GetDefaultFont()) {
        OutDrawList.PushClipRect(bar);
        OutDrawList.AddText(font, InName, Vec2(bar.Min().x + 6.0f, bar.Min().y + 4.0f), EditorStyle::FontSize - 2.0f,
                            EditorStyle::TextBright, m_WorldMatrix);
        OutDrawList.PopClipRect();
    }
}

void AnimationTransitionPreview::Paint(UIDrawList& OutDrawList) {
    OutDrawList.AddRect(m_Geometry, EditorStyle::PanelDark, m_WorldMatrix);
    OutDrawList.AddRect(UIRectF(Vec2(m_Geometry.Min().x, m_Geometry.Max().y - 1.0f), Vec2(m_Geometry.Size.x, 1.0f)),
                        EditorStyle::Border, m_WorldMatrix);
    Layout layout;
    if (!ComputeLayout(layout)) {
        return;
    }

    PaintRuler(OutDrawList, layout);
    PaintBar(OutDrawList, layout, layout.SourceBar, 0.0f, layout.SourceLength, 0.0f, layout.BlendEnd, layout.SourceName);
    PaintBar(OutDrawList, layout, layout.TargetBar, layout.TargetStart, layout.TargetLength, layout.ExitAt,
             layout.TargetStart + layout.TargetLength, layout.TargetName);

    const float blendLeft = layout.ToX(layout.ExitAt);
    const float blendRight = std::max(layout.ToX(layout.BlendEnd), blendLeft + 1.0f);
    const float top = layout.SourceBar.Min().y;
    const float bottom = layout.TargetBar.Max().y;
    OutDrawList.AddRect(UIRectF(Vec2(blendLeft, top), Vec2(blendRight - blendLeft, bottom - top)), s_Blend, m_WorldMatrix);
    OutDrawList.AddLine(Vec2(blendLeft, top), Vec2(blendRight, bottom), 1.5f, s_BlendEdge, m_WorldMatrix);

    const bool exitGrabbed = m_Dragged == Handle::ExitTime;
    const bool durationGrabbed = m_Dragged == Handle::Duration;
    OutDrawList.AddRect(UIRectF(Vec2(blendLeft - (exitGrabbed ? 1.0f : 0.5f), top), Vec2(exitGrabbed ? 2.0f : 1.0f, bottom - top)),
                        s_BlendEdge, m_WorldMatrix);
    OutDrawList.AddRect(UIRectF(Vec2(blendRight - (durationGrabbed ? 1.0f : 0.5f), top), Vec2(durationGrabbed ? 2.0f : 1.0f, bottom - top)),
                        s_BlendEdge, m_WorldMatrix);
}

AnimationTransitionPreview::Handle AnimationTransitionPreview::HandleAt(const Layout& InLayout, const Vec2& InCursorPos) const {
    const float blendLeft = InLayout.ToX(InLayout.ExitAt);
    const float blendRight = InLayout.ToX(InLayout.BlendEnd);
    const bool overBars = InCursorPos.y >= InLayout.SourceBar.Min().y && InCursorPos.y <= InLayout.TargetBar.Max().y;
    if (!overBars) {
        return Handle::None;
    }
    if (std::abs(InCursorPos.x - blendRight) <= s_GrabDistance) {
        return Handle::Duration;
    }
    if (InCursorPos.x >= blendLeft - s_GrabDistance && InCursorPos.x < blendRight) {
        return Handle::ExitTime;
    }
    const float targetLeft = InLayout.ToX(InLayout.TargetStart);
    const float targetRight = InLayout.ToX(InLayout.TargetStart + InLayout.TargetLength);
    if (HitTestRect(InLayout.TargetBar, InCursorPos) && InCursorPos.x >= targetLeft && InCursorPos.x <= targetRight) {
        return Handle::Offset;
    }
    return Handle::None;
}

void AnimationTransitionPreview::OnPressed(const Vec2& InCursorPos) {
    m_Dragged = Handle::None;
    AnimationTransition* transition = Transition.Get();
    Layout layout;
    if (!transition || !OwnerTab || !ComputeLayout(layout)) {
        return;
    }

    m_Dragged = HandleAt(layout, InCursorPos);
    if (m_Dragged == Handle::ExitTime && !transition->HasExitTime) {
        m_Dragged = Handle::None;
    }
    m_PressX = InCursorPos.x;
    m_SecondsPerPixel = layout.SecondsPerPixel();
    m_SourceLength = layout.SourceLength;
    m_TargetLength = layout.TargetLength;
    switch (m_Dragged) {
        case Handle::None:     return;
        case Handle::ExitTime: m_PressValue = transition->ExitTime; break;
        case Handle::Duration: m_PressValue = transition->GetDurationSeconds(layout.SourceLength); break;
        case Handle::Offset:   m_PressValue = transition->Offset; break;
    }
    OwnerTab->RecordEdit("Edit Transition", transition);
}

void AnimationTransitionPreview::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    AnimationTransition* transition = Transition.Get();
    if (!transition) {
        return;
    }
    const float seconds = (InCursorPos.x - m_PressX) * m_SecondsPerPixel;
    switch (m_Dragged) {
        case Handle::None:
            break;
        case Handle::ExitTime:
            transition->ExitTime = std::clamp(m_PressValue + seconds / m_SourceLength, 0.0f, 10.0f);
            break;
        case Handle::Duration: {
            const float duration = std::max(m_PressValue + seconds, 0.0f);
            transition->Duration = std::min(transition->FixedDuration ? duration : duration / m_SourceLength, 10.0f);
            break;
        }
        case Handle::Offset:
            transition->Offset = std::clamp(m_PressValue - seconds / m_TargetLength, 0.0f, 1.0f);
            break;
    }
}

void AnimationTransitionPreview::OnReleased(bool InInside) {
    (void)InInside;
    static const char* s_Names[] = { "", "ExitTime", "Duration", "Offset" };
    if (m_Dragged != Handle::None && OwnerTab && Transition.Get()) {
        DetailsCustomization::NotifyPropertyEdited(OwnerTab, Transition.Get(), s_Names[(int32_t)m_Dragged]);
    }
    m_Dragged = Handle::None;
}
