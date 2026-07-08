#include "UINode.h"
#include "Rendering/UIDrawList.h"
#include <cmath>

UIRectF UINode::ComputeGeometry(const UIRectF& InParentContentRect) const {
    // Place the node so its Pivot lands on Anchor + Position within the parent's content rect.
    const Vec2 parentSize = InParentContentRect.Size;
    const Vec2 size = Size.Resolve(parentSize);
    const Vec2 pivotPos = InParentContentRect.Min() + Anchor * parentSize + Position.Resolve(parentSize);
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
    m_Geometry = ComputeGeometry(InParentContentRect);

    // Rotate this node (and, through m_WorldMatrix, its children) about its pivot, on top of the
    // parent's transform. Layout stays axis-aligned; rotation/perspective apply at paint/hit-test.
    const Vec2 pivotPx = m_Geometry.Min() + Pivot * m_Geometry.Size;
    m_WorldMatrix = InParentWorld * RotationAbout(pivotPx, Rotation);

    LayoutChildren(GetContentRect());
}

void UINode::LayoutChildren(const UIRectF& InContent) {
    for (UINode* child : CollectUIChildren()) {
        child->Layout(InContent, m_WorldMatrix);
    }
}

Array<UINode*> UINode::CollectUIChildren() const {
    Array<UINode*> children;
    for (uint32_t i = 0; i < GetChildCount(); i++) {
        if (UINode* child = GetChild((int)i)->As<UINode>()) {
            children.Add(child);
        }
    }
    return children;
}

void UINode::Paint(UIDrawList& OutDrawList) {
    (void)OutDrawList;
}

void UINode::BindTree() {
    if (!IsEnabled()) {
        return;
    }
    // Bind/reconcile this node first so containers can add children that also bind this frame.
    OnBind();
    for (uint32_t i = 0; i < GetChildCount(); i++) {
        if (UINode* child = GetChild((int)i)->As<UINode>()) {
            child->BindTree();
        }
    }
}

void UINode::PaintTree(UIDrawList& OutDrawList) {
    if (!IsEnabled()) {
        return;
    }
    Paint(OutDrawList);
    if (ClipChildren) {
        OutDrawList.PushClipRect(GetContentRect());
    }
    for (uint32_t i = 0; i < GetChildCount(); i++) {
        if (UINode* child = GetChild((int)i)->As<UINode>()) {
            child->PaintTree(OutDrawList);
        }
    }
    if (ClipChildren) {
        OutDrawList.PopClipRect();
    }
    PaintOverlay(OutDrawList);
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

Vec2 UINode::ProjectToScreen(const Vec3& InCanvasPixelPos) {
    // Run the same canvas->clip matrix the GPU uses, then map NDC to screen pixels (Vulkan NDC
    // and the mouse share a top-left origin, so no Y flip).
    const Vec4 clip = s_ViewProjection * Vec4(InCanvasPixelPos, 1.0f);
    if (clip.w <= 1e-4f) {
        return Vec2(-1e6f);  // behind the camera / past the perspective pole: never hit
    }
    return Vec2((clip.x / clip.w * 0.5f + 0.5f) * s_ViewportW,
                (clip.y / clip.w * 0.5f + 0.5f) * s_ViewportH);
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
    return HitTestRect(m_Geometry, InPoint);
}

Vec2 UINode::LocalToScreen(const Vec2& InLocalPoint) const {
    const Vec4 world = m_WorldMatrix * Vec4(InLocalPoint, 0.0f, 1.0f);
    return ProjectToScreen(Vec3(world.x, world.y, world.z));
}

bool UINode::HitTestRect(const UIRectF& InRect, const Vec2& InPoint) const {
    const Vec2 mn = InRect.Min();
    const Vec2 mx = InRect.Max();
    const Vec2 corners[4] = {
        LocalToScreen(Vec2(mn.x, mn.y)), LocalToScreen(Vec2(mn.x, mx.y)),
        LocalToScreen(Vec2(mx.x, mx.y)), LocalToScreen(Vec2(mx.x, mn.y))
    };
    return PointInQuad(InPoint, corners);
}
