#include "UICanvas.h"
#include "Rendering/UIDrawList.h"
#include "GameFramework/CameraNode.h"
#include <algorithm>
#include <cmath>

UICanvas::UICanvas() {
    Fill();
}

float UICanvas::ComputeScaleFactor(const Vec2& InViewportSize) const {
    if (ScaleMode == UICanvasScaleMode::ScaleWithScreenSize) {
        // Blend the width- and height-relative factors in log space so MatchWidthHeight = 0.5
        // treats "twice as wide" and "twice as tall" symmetrically (Unity's CanvasScaler formula).
        const float logW = std::log2(InViewportSize.x / ReferenceResolution.x);
        const float logH = std::log2(InViewportSize.y / ReferenceResolution.y);
        return std::exp2(glm::mix(logW, logH, MatchWidthHeight));
    }
    return ScaleFactor;
}

UIRectF UICanvas::ComputeCanvasRect(const Vec2& InViewportSize) const {
    if (RenderMode == UICanvasRenderMode::World) {
        return UIRectF(Vec2(0.0f), CanvasSize);
    }
    const float scale = std::max(ComputeScaleFactor(InViewportSize), 1e-4f);
    return UIRectF(Vec2(0.0f), InViewportSize / scale);
}

// Maps canvas pixel-space (x in [0,W], y in [0,H], z = depth from tilt) to clip space with a
// perspective divide about the canvas centre. At z=0 it is the plain pixel->NDC mapping.
static Mat4 BuildOverlayProjection(float InWidth, float InHeight, float InPerspective) {
    Mat4 m(0.0f);
    m[0][0] = 2.0f / InWidth;         // clip.x = 2x/W - 1
    m[1][1] = 2.0f / InHeight;        // clip.y = 2y/H - 1
    m[3][0] = -1.0f;
    m[3][1] = -1.0f;
    m[2][3] = -1.0f / InPerspective;  // clip.w = 1 - z/P
    m[3][3] = 1.0f;
    return m;
}

Mat4 UICanvas::BuildProjection(const Vec2& InViewportSize) const {
    if (RenderMode == UICanvasRenderMode::World) {
        // Canvas pixels (top-left origin, +Y down) -> canvas-local units (centred, +Y up).
        const float pixelsPerUnit = std::max(PixelsPerUnit, 1e-4f);
        const Mat4 pxToLocal =
            glm::scale(Mat4(1.0f), Vec3(1.0f, -1.0f, 1.0f) / pixelsPerUnit) *
            glm::translate(Mat4(1.0f), Vec3(-CanvasSize.x * 0.5f, -CanvasSize.y * 0.5f, 0.0f));

        Mat4 rot(1.0f);
        rot = glm::rotate(rot, glm::radians(WorldRotation.z), Vec3(0.0f, 0.0f, 1.0f));
        rot = glm::rotate(rot, glm::radians(WorldRotation.y), Vec3(0.0f, 1.0f, 0.0f));
        rot = glm::rotate(rot, glm::radians(WorldRotation.x), Vec3(1.0f, 0.0f, 0.0f));
        const Mat4 localToWorld = glm::translate(Mat4(1.0f), WorldPosition) * rot;

        const Mat4 viewProjection = Camera ? Camera->GetViewProjectionMatrix() : Mat4(1.0f);
        return viewProjection * localToWorld * pxToLocal;
    }

    // Overlay: project canvas units straight to the viewport; the scale is implicit in the
    // canvas rect being viewport / scale. Perspective (for tilted nodes) is in canvas units.
    const UIRectF rect = ComputeCanvasRect(InViewportSize);
    return BuildOverlayProjection(rect.Size.x, rect.Size.y, Perspective);
}

Mat4 UICanvas::RunFrame(const Vec2& InViewportSize, const UIFrameContext& InContext, UIDrawList& OutDrawList) {
    const Mat4 projection = BuildProjection(InViewportSize);
    SetViewProjection(projection, InViewportSize.x, InViewportSize.y);

    BindTree();
    Layout(ComputeCanvasRect(InViewportSize));
    RouteInput(InContext);
    UpdateTree(InContext);
    PaintTree(OutDrawList);
    return projection;
}

void UICanvas::SetFocus(UINode* InNode) {
    UINode* previous = m_FocusedNode.Get();
    if (previous == InNode) {
        return;
    }
    if (previous) {
        previous->m_Focused = false;
        previous->OnFocusChanged(false);
    }
    m_FocusedNode = InNode;
    if (InNode) {
        InNode->m_Focused = true;
        InNode->OnFocusChanged(true);
    }
}

void UICanvas::SetHovered(UINode* InNode) {
    UINode* previous = m_HoveredNode.Get();
    if (previous == InNode) {
        return;
    }
    if (previous) {
        previous->m_Hovered = false;
        previous->OnHoverChanged(false);
    }
    m_HoveredNode = InNode;
    if (InNode) {
        InNode->m_Hovered = true;
        InNode->OnHoverChanged(true);
    }
}

UINode* UICanvas::HitTestTopmost(UINode* InNode, const Vec2& InPoint, bool InInteractableOnly) {
    if (!InNode->IsEnabled()) {
        return nullptr;
    }
    // Later-painted children are on top, so search children (reverse) before self.
    if (!InNode->ClipChildren || InNode->HitTestRect(InNode->GetContentRect(), InPoint)) {
        for (int i = (int)InNode->GetChildCount() - 1; i >= 0; i--) {
            if (UINode* child = InNode->GetChild(i)->As<UINode>()) {
                if (UINode* hit = HitTestTopmost(child, InPoint, InInteractableOnly)) {
                    return hit;
                }
            }
        }
    }
    if ((!InInteractableOnly || InNode->Interactable) && InNode->HitTest(InPoint)) {
        return InNode;
    }
    return nullptr;
}

void UICanvas::RouteInput(const UIFrameContext& InContext) {
    if (InputMode == UIInputMode::Cursor) {
        RouteCursor(InContext);
    } else {
        RouteFocus(InContext);
    }
}

void UICanvas::RouteCursor(const UIFrameContext& InContext) {
    const Vec2 cursor = InContext.CursorPosition;
    UINode* hit = HitTestTopmost(this, cursor, true);

    // A captured node keeps hover even when the cursor leaves it.
    SetHovered(m_CapturedNode.Get() ? m_CapturedNode.Get() : hit);

    if (InContext.CursorPressedThisFrame && hit) {
        m_CapturedNode = hit;
        SetFocus(hit);
        hit->m_Pressed = true;
        hit->OnPressed(cursor);
    }

    if (UINode* captured = m_CapturedNode.Get()) {
        if (cursor != m_LastCursor) {
            captured->OnDrag(cursor, cursor - m_LastCursor);
        }
        if (InContext.CursorReleasedThisFrame) {
            const bool inside = captured->HitTest(cursor);
            captured->m_Pressed = false;
            captured->OnReleased(inside);
            if (inside) {
                captured->OnClick();
            }
            m_CapturedNode = nullptr;
        }
    }

    if (InContext.ScrollDelta != Vec2(0.0f)) {
        // Bubble up from the innermost node under the cursor until one consumes it.
        for (Node* node = HitTestTopmost(this, cursor, false); node; node = node->GetParent()) {
            UINode* uiNode = node->As<UINode>();
            if (uiNode && uiNode->OnScroll(InContext.ScrollDelta)) {
                break;
            }
        }
    }

    m_LastCursor = cursor;
}

// All enabled interactable nodes below InNode, skipping subtrees hidden by a clipping ancestor.
static void CollectFocusable(UINode* InNode, const UIRectF* InClip, Array<UINode*>& OutNodes) {
    if (!InNode->IsEnabled()) {
        return;
    }
    const UIRectF& geometry = InNode->GetGeometry();
    const bool visible = !InClip
        || (geometry.Max().x > InClip->Min().x && geometry.Min().x < InClip->Max().x
         && geometry.Max().y > InClip->Min().y && geometry.Min().y < InClip->Max().y);
    if (visible && InNode->Interactable) {
        OutNodes.Add(InNode);
    }
    UIRectF clipStorage;
    const UIRectF* childClip = InClip;
    if (InNode->ClipChildren) {
        clipStorage = InNode->GetContentRect();  // nested clips are not intersected here
        childClip = &clipStorage;
    }
    for (uint32_t i = 0; i < InNode->GetChildCount(); i++) {
        if (UINode* child = InNode->GetChild((int)i)->As<UINode>()) {
            CollectFocusable(child, childClip, OutNodes);
        }
    }
}

UINode* UICanvas::FindNavTarget(UINode* InFrom, UINavDirection InDirection) const {
    Array<UINode*> candidates;
    CollectFocusable(const_cast<UICanvas*>(this), nullptr, candidates);
    if (candidates.IsEmpty()) {
        return nullptr;
    }
    if (!InFrom) {
        return candidates[0];
    }

    static const Vec2 s_Directions[4] = { Vec2(0, -1), Vec2(0, 1), Vec2(-1, 0), Vec2(1, 0) };
    const Vec2 direction = s_Directions[(int)InDirection];
    const Vec2 from = InFrom->GetGeometry().Center();

    // Nearest candidate ahead in the nav direction, penalizing sideways offset.
    UINode* best = nullptr;
    float bestScore = 0.0f;
    for (UINode* candidate : candidates) {
        if (candidate == InFrom) {
            continue;
        }
        const Vec2 delta = candidate->GetGeometry().Center() - from;
        const float ahead = delta.x * direction.x + delta.y * direction.y;
        if (ahead <= 0.001f) {
            continue;
        }
        const Vec2 side = delta - direction * ahead;
        const float score = ahead + 2.0f * std::abs(side.x + side.y);
        if (!best || score < bestScore) {
            best = candidate;
            bestScore = score;
        }
    }
    return best;
}

void UICanvas::RouteFocus(const UIFrameContext& InContext) {
    UINode* focused = m_FocusedNode.Get();

    const bool navPressed = InContext.NavUp || InContext.NavDown || InContext.NavLeft || InContext.NavRight;
    if (!focused && (navPressed || InContext.NavSelectPressed)) {
        SetFocus(FindNavTarget(nullptr, UINavDirection::Down));
        focused = m_FocusedNode.Get();
        SetHovered(focused);
        return;  // first input just establishes focus
    }

    if (navPressed) {
        UINavDirection direction = UINavDirection::Down;
        if (InContext.NavUp) direction = UINavDirection::Up;
        else if (InContext.NavLeft) direction = UINavDirection::Left;
        else if (InContext.NavRight) direction = UINavDirection::Right;
        if (UINode* target = FindNavTarget(focused, direction)) {
            SetFocus(target);
            focused = target;
        }
    }

    SetHovered(focused);

    if (focused) {
        if (InContext.NavSelectPressed) {
            focused->m_Pressed = true;
            focused->OnPressed(focused->GetGeometry().Center());  // no pointer in Focus mode
        }
        if (InContext.NavSelectReleased && focused->IsPressed()) {
            focused->m_Pressed = false;
            focused->OnReleased(true);
            focused->OnClick();
        }
        if (InContext.NavBack) {
            for (Node* node = focused; node; node = node->GetParent()) {
                UINode* uiNode = node->As<UINode>();
                if (uiNode && uiNode->OnNavBack()) {
                    break;
                }
            }
        }
    }
}
