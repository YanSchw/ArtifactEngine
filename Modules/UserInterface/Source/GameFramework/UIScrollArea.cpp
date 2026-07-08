#include "UIScrollArea.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

UIScrollArea::UIScrollArea() {
    ClipChildren = true;
    Interactable = true;  // so the router delivers wheel/press/drag here
}

bool UIScrollArea::OnScroll(const Vec2& InDelta) {
    ScrollOffset -= InDelta * ScrollSpeed;
    ClampScrollOffset();
    return true;
}

void UIScrollArea::ClampScrollOffset() {
    const Vec2 contentSize = GetContentRect().Size;
    const Vec2 maxOffset = Vec2(std::max(0.0f, m_ContentSize.x - contentSize.x),
                                std::max(0.0f, m_ContentSize.y - contentSize.y));
    ScrollOffset = Vec2(std::clamp(ScrollOffset.x, 0.0f, maxOffset.x),
                        std::clamp(ScrollOffset.y, 0.0f, maxOffset.y));
}

// Furthest bottom-right corner reached by any node in the subtree.
static void MaxSubtreeCorner(Node* InNode, Vec2& OutMax) {
    for (uint32_t i = 0; i < InNode->GetChildCount(); i++) {
        if (UINode* child = InNode->GetChild((int)i)->As<UINode>()) {
            if (!child->IsEnabled()) {
                continue;
            }
            const Vec2 corner = child->GetGeometry().Max();
            OutMax = Vec2(std::max(OutMax.x, corner.x), std::max(OutMax.y, corner.y));
            MaxSubtreeCorner(child, OutMax);
        }
    }
}

void UIScrollArea::LayoutChildren(const UIRectF& InContent) {
    // Clamp against last frame's m_ContentSize
    // OnScroll/OnDrag reclamp on change so nothing overshoots.
    ClampScrollOffset();

    UIRectF scrolled = InContent;
    scrolled.Position -= ScrollOffset;
    for (UINode* child : CollectUIChildren()) {
        LayoutChild(*child, scrolled, m_WorldMatrix);
    }

    Vec2 maxCorner = scrolled.Min();
    MaxSubtreeCorner(this, maxCorner);
    m_ContentSize = maxCorner - scrolled.Min();
}

UIRectF UIScrollArea::ComputeVerticalThumbRect(const UIRectF& InContent) const {
    if (m_ContentSize.y <= InContent.Size.y || InContent.Size.y <= 0.0f) {
        return UIRectF();
    }
    const float thumbH = std::max(16.0f, InContent.Size.y * InContent.Size.y / m_ContentSize.y);
    const float maxOffset = m_ContentSize.y - InContent.Size.y;
    const float t = (maxOffset > 0.0f) ? (ScrollOffset.y / maxOffset) : 0.0f;
    const float thumbY = InContent.Position.y + (InContent.Size.y - thumbH) * t;
    return UIRectF(Vec2(InContent.Max().x - ScrollbarWidth, thumbY), Vec2(ScrollbarWidth, thumbH));
}

UIRectF UIScrollArea::ComputeHorizontalThumbRect(const UIRectF& InContent) const {
    if (m_ContentSize.x <= InContent.Size.x || InContent.Size.x <= 0.0f) {
        return UIRectF();
    }
    const float thumbW = std::max(16.0f, InContent.Size.x * InContent.Size.x / m_ContentSize.x);
    const float maxOffset = m_ContentSize.x - InContent.Size.x;
    const float t = (maxOffset > 0.0f) ? (ScrollOffset.x / maxOffset) : 0.0f;
    const float thumbX = InContent.Position.x + (InContent.Size.x - thumbW) * t;
    return UIRectF(Vec2(thumbX, InContent.Max().y - ScrollbarWidth), Vec2(thumbW, ScrollbarWidth));
}

void UIScrollArea::PaintOverlay(UIDrawList& OutDrawList) {
    if (ScrollbarColor.a <= 0.0f) {
        return;
    }
    const UIRectF content = GetContentRect();

    const UIRectF vThumb = ComputeVerticalThumbRect(content);
    if (vThumb.Size.y > 0.0f) {
        OutDrawList.AddRect(vThumb, ScrollbarColor, m_WorldMatrix);
    }
    const UIRectF hThumb = ComputeHorizontalThumbRect(content);
    if (hThumb.Size.x > 0.0f) {
        OutDrawList.AddRect(hThumb, ScrollbarColor, m_WorldMatrix);
    }
}

float UIScrollArea::ScreenPosToLocalAxis(const Vec2& InScreenPos, const UIRectF& InContent, int InAxis) const {
    const Vec2 a = InContent.Min();
    Vec2 b = a;
    if (InAxis == 0) b.x = InContent.Max().x; else b.y = InContent.Max().y;

    const Vec2 screenA = LocalToScreen(a);
    const Vec2 screenB = LocalToScreen(b);
    const Vec2 axis = screenB - screenA;
    const float lenSq = glm::dot(axis, axis);
    if (lenSq < 1e-6f) {
        return InAxis == 0 ? a.x : a.y;
    }
    const float t = std::clamp(glm::dot(InScreenPos - screenA, axis) / lenSq, 0.0f, 1.0f);
    return (InAxis == 0 ? a.x : a.y) + t * (InAxis == 0 ? (b.x - a.x) : (b.y - a.y));
}

void UIScrollArea::OnPressed(const Vec2& InCursorPos) {
    const UIRectF content = GetContentRect();
    const UIRectF vThumb = ComputeVerticalThumbRect(content);
    const UIRectF hThumb = ComputeHorizontalThumbRect(content);

    if (vThumb.Size.y > 0.0f && HitTestRect(vThumb, InCursorPos)) {
        m_DragTarget = EDragTarget::VerticalThumb;
        m_DragGrabOffset = ScreenPosToLocalAxis(InCursorPos, content, 1) - vThumb.Position.y;
    } else if (hThumb.Size.x > 0.0f && HitTestRect(hThumb, InCursorPos)) {
        m_DragTarget = EDragTarget::HorizontalThumb;
        m_DragGrabOffset = ScreenPosToLocalAxis(InCursorPos, content, 0) - hThumb.Position.x;
    } else {
        m_DragTarget = EDragTarget::None;
    }
}

void UIScrollArea::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (m_DragTarget == EDragTarget::None) {
        return;
    }
    const UIRectF content = GetContentRect();

    if (m_DragTarget == EDragTarget::VerticalThumb) {
        const float thumbH = ComputeVerticalThumbRect(content).Size.y;
        const float travel = content.Size.y - thumbH;
        const float maxOffset = m_ContentSize.y - content.Size.y;
        if (travel > 0.0f && maxOffset > 0.0f) {
            const float thumbY = ScreenPosToLocalAxis(InCursorPos, content, 1) - m_DragGrabOffset;
            const float t = std::clamp((thumbY - content.Position.y) / travel, 0.0f, 1.0f);
            ScrollOffset.y = t * maxOffset;
        }
    } else {
        const float thumbW = ComputeHorizontalThumbRect(content).Size.x;
        const float travel = content.Size.x - thumbW;
        const float maxOffset = m_ContentSize.x - content.Size.x;
        if (travel > 0.0f && maxOffset > 0.0f) {
            const float thumbX = ScreenPosToLocalAxis(InCursorPos, content, 0) - m_DragGrabOffset;
            const float t = std::clamp((thumbX - content.Position.x) / travel, 0.0f, 1.0f);
            ScrollOffset.x = t * maxOffset;
        }
    }
}

void UIScrollArea::OnReleased(bool InInside) {
    (void)InInside;
    m_DragTarget = EDragTarget::None;
}
