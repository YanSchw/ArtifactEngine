#include "UINode.h"
#include "Rendering/UIDrawList.h"
#include <cmath>

UIRectF UINode::ComputeGeometry(const UIRectF& InParentContentRect) const {
    const Vec2 parentMin = InParentContentRect.Min();
    const Vec2 parentSize = InParentContentRect.Size;

    const Vec2 anchorRefMin = parentMin + AnchorMin * parentSize;
    const Vec2 anchorRefMax = parentMin + AnchorMax * parentSize;
    const Vec2 anchorRefSize = anchorRefMax - anchorRefMin;

    const Vec2 size = anchorRefSize + SizeDelta;
    const Vec2 pivotPos = anchorRefMin + Pivot * anchorRefSize + AnchoredPosition;
    return UIRectF(pivotPos - Pivot * size, size);
}

// Euler (degrees) rotation about a pivot in the z=0 plane, as a 4x4 affine transform.
static Mat4 RotationAbout(const Vec2& InPivot, const Vec3& InEulerDegrees) {
    const Vec3 pivot(InPivot, 0.0f);
    Mat4 rot(1.0f);
    rot = glm::rotate(rot, glm::radians(InEulerDegrees.z), Vec3(0.0f, 0.0f, 1.0f));
    rot = glm::rotate(rot, glm::radians(InEulerDegrees.y), Vec3(0.0f, 1.0f, 0.0f));
    rot = glm::rotate(rot, glm::radians(InEulerDegrees.x), Vec3(1.0f, 0.0f, 0.0f));
    return glm::translate(Mat4(1.0f), pivot) * rot * glm::translate(Mat4(1.0f), -pivot);
}

void UINode::Layout(const UIRectF& InParentContentRect, const Mat4& InParentWorld) {
    m_Geometry = ComputeGeometry(InParentContentRect).Deflate(Margin);

    // Rotate this node (and, through m_WorldMatrix, its children) about its pivot, on top of the
    // parent's transform. Layout stays axis-aligned; rotation/perspective apply at paint/hit-test.
    const Vec2 pivotPx = m_Geometry.Min() + Pivot * m_Geometry.Size;
    m_WorldMatrix = InParentWorld * RotationAbout(pivotPx, Rotation);

    Array<UINode*> children;
    for (uint32_t i = 0; i < GetChildCount(); i++) {
        if (UINode* child = GetChild((int)i)->As<UINode>()) {
            children.Add(child);
        }
    }

    const UIRectF content = GetContentRect();

    if (LayoutMode == UILayoutMode::None || children.IsEmpty()) {
        for (UINode* child : children) {
            child->Layout(content, m_WorldMatrix);
        }
        return;
    }

    // Split: distribute the content rect equally along one axis, separated by Gap.
    const int32_t count = children.Size();
    const float totalGap = Gap * (float)(count - 1);

    if (LayoutMode == UILayoutMode::SplitX) {
        const float slotWidth = std::max(0.0f, (content.Size.x - totalGap) / (float)count);
        float x = content.Position.x;
        for (UINode* child : children) {
            child->Layout(UIRectF(Vec2(x, content.Position.y), Vec2(slotWidth, content.Size.y)), m_WorldMatrix);
            x += slotWidth + Gap;
        }
    } else {
        const float slotHeight = std::max(0.0f, (content.Size.y - totalGap) / (float)count);
        float y = content.Position.y;
        for (UINode* child : children) {
            child->Layout(UIRectF(Vec2(content.Position.x, y), Vec2(content.Size.x, slotHeight)), m_WorldMatrix);
            y += slotHeight + Gap;
        }
    }
}

void UINode::Paint(UIDrawList& OutDrawList) {
    if (BackgroundColor.a > 0.0f) {
        OutDrawList.AddRect(m_Geometry, BackgroundColor, m_WorldMatrix);
    }
}

void UINode::PaintTree(UIDrawList& OutDrawList) {
    if (!IsEnabled()) {
        return;
    }
    Paint(OutDrawList);
    for (uint32_t i = 0; i < GetChildCount(); i++) {
        if (UINode* child = GetChild((int)i)->As<UINode>()) {
            child->PaintTree(OutDrawList);
        }
    }
}

void UINode::UpdateTree(const UIFrameContext& InContext) {
    if (!IsEnabled()) {
        return;
    }
    OnUIUpdate(InContext);
    for (uint32_t i = 0; i < GetChildCount(); i++) {
        if (UINode* child = GetChild((int)i)->As<UINode>()) {
            child->UpdateTree(InContext);
        }
    }
}

Vec2 UINode::ProjectToScreen(const Vec3& InWorldPixelPos) {
    // Perspective divide about the viewport centre (CSS-like): factor = P / (P - z).
    const float halfW = s_ViewportW * 0.5f;
    const float halfH = s_ViewportH * 0.5f;
    const float denom = s_Perspective - InWorldPixelPos.z;
    const float factor = (std::abs(denom) < 1e-3f) ? 1.0f : (s_Perspective / denom);
    return Vec2(halfW + (InWorldPixelPos.x - halfW) * factor,
                halfH + (InWorldPixelPos.y - halfH) * factor);
}

// Point-in-convex-quad via consistent edge-cross signs. Corners in order TL, BL, BR, TR.
static bool PointInQuad(const Vec2& InP, const Vec2 InCorners[4]) {
    float sign = 0.0f;
    for (int i = 0; i < 4; i++) {
        const Vec2& a = InCorners[i];
        const Vec2& b = InCorners[(i + 1) % 4];
        const float cross = (b.x - a.x) * (InP.y - a.y) - (b.y - a.y) * (InP.x - a.x);
        if (std::abs(cross) < 1e-6f) continue;
        if (sign == 0.0f) sign = cross;
        else if ((cross > 0.0f) != (sign > 0.0f)) return false;
    }
    return true;
}

bool UINode::HitTest(const Vec2& InPoint) const {
    const Vec2 mn = m_Geometry.Min();
    const Vec2 mx = m_Geometry.Max();
    const Vec3 local[4] = {
        Vec3(mn.x, mn.y, 0.0f), Vec3(mn.x, mx.y, 0.0f),
        Vec3(mx.x, mx.y, 0.0f), Vec3(mx.x, mn.y, 0.0f)
    };
    Vec2 corners[4];
    for (int i = 0; i < 4; i++) {
        const Vec4 world = m_WorldMatrix * Vec4(local[i], 1.0f);
        corners[i] = ProjectToScreen(Vec3(world.x, world.y, world.z));
    }
    return PointInQuad(InPoint, corners);
}
