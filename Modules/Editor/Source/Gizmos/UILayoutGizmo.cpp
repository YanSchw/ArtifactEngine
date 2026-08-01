#include "UILayoutGizmo.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UIStack.h"
#include "Rendering/UIDrawList.h"
#include "Assets/Font.h"
#include "InputSystem/KeyboardDevice.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <cstdio>

static constexpr float s_HandleHalf = 4.0f;
static constexpr float s_HandleSlop = 7.0f;
static constexpr float s_PivotRadius = 5.5f;
static constexpr float s_AnchorArm = 9.0f;
static constexpr float s_OutlineWidth = 1.0f;
static constexpr float s_SnapThreshold = 6.0f;
static constexpr float s_GridStep = 8.0f;
static constexpr float s_FractionSnapPixels = 6.0f;
static constexpr float s_RotateSnapDegrees = 15.0f;
static constexpr float s_MinExtent = 1.0f;
static constexpr float s_ReadoutSize = 12.0f;

static const Vec4 s_OutlineColor = EditorStyle::AccentBright;
static const Vec4 s_HandleColor = HexColor(0xFFFFFF);
static const Vec4 s_HandleHoverColor = HexColor(0xFFC02E);
static const Vec4 s_StackHandleColor = HexColor(0x8F8F8F);
static const Vec4 s_ParentColor = HexColor(0x6E7A86, 0.55f);
static const Vec4 s_AnchorColor = HexColor(0xF5D06A);
static const Vec4 s_PivotColor = HexColor(0x33E0A0);
static const Vec4 s_GuideColor = HexColor(0xFF4FA3);
static const Vec4 s_HoverColor = HexColor(0x26BBFF, 0.5f);
static const Vec4 s_ReadoutBackground = HexColor(0x101010, 0.85f);

static bool IsKeyHeld(KeyCode InPrimary, KeyCode InSecondary) {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && (keyboard->IsPressed(InPrimary) || keyboard->IsPressed(InSecondary));
}

static bool IsGridSnapping() { return IsKeyHeld(KeyCode::LeftControl, KeyCode::RightControl); }
static bool IsConstraining() { return IsKeyHeld(KeyCode::LeftShift, KeyCode::RightShift); }
static bool IsRelativeEditing() { return IsKeyHeld(KeyCode::LeftAlt, KeyCode::RightAlt); }

static float SnapToStep(float InValue, float InStep) {
    return std::round(InValue / InStep) * InStep;
}

static float SnapToStops(float InValue, const float* InStops, int32_t InCount, float InThreshold) {
    float best = InValue;
    float bestDistance = InThreshold;
    for (int32_t i = 0; i < InCount; i++) {
        const float distance = std::abs(InValue - InStops[i]);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = InStops[i];
        }
    }
    return best;
}

static bool IsRotated(UINode* InNode) {
    return InNode && glm::dot(InNode->Rotation, InNode->Rotation) > 1e-8f;
}

static float SafeExtent(float InValue) {
    return std::abs(InValue) < 1e-4f ? 1e-4f : InValue;
}

static String FormatNumber(float InValue) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.4g", InValue);
    return String(buffer);
}

static void ResizeSigns(UILayoutGizmo::Handle InHandle, float& OutX, float& OutY) {
    OutX = 0.0f;
    OutY = 0.0f;
    switch (InHandle) {
        case UILayoutGizmo::Handle::Left:        OutX = -1.0f; break;
        case UILayoutGizmo::Handle::Right:       OutX =  1.0f; break;
        case UILayoutGizmo::Handle::Top:         OutY = -1.0f; break;
        case UILayoutGizmo::Handle::Bottom:      OutY =  1.0f; break;
        case UILayoutGizmo::Handle::TopLeft:     OutX = -1.0f; OutY = -1.0f; break;
        case UILayoutGizmo::Handle::TopRight:    OutX =  1.0f; OutY = -1.0f; break;
        case UILayoutGizmo::Handle::BottomRight: OutX =  1.0f; OutY =  1.0f; break;
        case UILayoutGizmo::Handle::BottomLeft:  OutX = -1.0f; OutY =  1.0f; break;
        default: break;
    }
}

Vec2 UILayoutGizmo::Transform(const Mat4& InMatrix, const Vec2& InPoint) {
    const Vec4 result = InMatrix * Vec4(InPoint.x, InPoint.y, 0.0f, 1.0f);
    const float w = std::abs(result.w) < 1e-6f ? 1.0f : result.w;
    return Vec2(result.x / w, result.y / w);
}

UINode* UILayoutGizmo::ParentNodeOf(UINode* InNode) {
    if (!InNode || !InNode->GetParent()) {
        return nullptr;
    }
    return InNode->GetParent()->As<UINode>();
}

UIRectF UILayoutGizmo::ParentContentRect(UINode* InNode) const {
    if (!InNode) {
        return UIRectF();
    }
    UINode* parent = ParentNodeOf(InNode);
    return parent ? parent->GetContentRect() : m_RootRect;
}

Mat4 UILayoutGizmo::ParentWorldMatrix(UINode* InNode) {
    UINode* parent = ParentNodeOf(InNode);
    return parent ? parent->GetWorldMatrix() : Mat4(1.0f);
}

bool UILayoutGizmo::IsInDesign(UINode* InNode) const {
    if (!InNode || InNode->As<UICanvas>()) {
        return false;
    }
    for (const WeakObjectPtr<UINode>& weak : m_Roots) {
        UINode* root = weak.Get();
        if (root && (root == InNode || InNode->IsChildOf(root))) {
            return true;
        }
    }
    return false;
}

int32_t UILayoutGizmo::StackAxisOf(UINode* InNode) {
    if (!InNode || !InNode->GetParent()) {
        return -1;
    }
    UIStack* stack = InNode->GetParent()->As<UIStack>();
    return stack ? (stack->Axis == UIAxis::X ? 0 : 1) : -1;
}

void UILayoutGizmo::Update(MajorTab* InMajorTab, const Array<UINode*>& InRoots, const UIRectF& InRootRect, float InPixelScale) {
    m_Roots.Clear();
    for (UINode* root : InRoots) {
        m_Roots.Add(WeakObjectPtr<UINode>(root));
    }
    m_RootRect = InRootRect;
    m_PixelScale = InPixelScale > 1e-4f ? InPixelScale : 1.0f;
    if (IsDragging()) {
        return;
    }

    m_Targets.Clear();
    m_Primary = nullptr;
    if (!InMajorTab) {
        m_Hover = Handle::None;
        return;
    }

    Array<UINode*> selected;
    for (Object* object : InMajorTab->GetSelection()) {
        UINode* node = Cast<UINode>(object);
        if (IsInDesign(node)) {
            selected.Add(node);
        }
    }

    for (UINode* node : selected) {
        bool hasSelectedAncestor = false;
        for (UINode* other : selected) {
            if (other != node && node->IsChildOf(other)) {
                hasSelectedAncestor = true;
                break;
            }
        }
        if (!hasSelectedAncestor) {
            m_Targets.Add(WeakObjectPtr<UINode>(node));
        }
    }
    if (!m_Targets.IsEmpty()) {
        m_Primary = m_Targets.LastItem();
    } else {
        m_Hover = Handle::None;
    }
}

bool UILayoutGizmo::IsActive() const {
    return Tool != UILayoutTool::Select && m_Primary.Get() != nullptr;
}

UINode* UILayoutGizmo::PickRecursive(UINode* InNode, const Vec2& InCanvasPoint) const {
    if (!InNode || !InNode->IsEnabled()) {
        return nullptr;
    }
    const Vec2 local = Transform(glm::inverse(InNode->GetWorldMatrix()), InCanvasPoint);
    if (!InNode->ClipChildren || InNode->GetContentRect().Contains(local)) {
        for (int32_t i = (int32_t)InNode->GetChildCount() - 1; i >= 0; i--) {
            if (UINode* child = InNode->GetChild(i)->As<UINode>()) {
                if (UINode* hit = PickRecursive(child, InCanvasPoint)) {
                    return hit;
                }
            }
        }
    }
    return InNode->GetGeometry().Contains(local) ? InNode : nullptr;
}

UINode* UILayoutGizmo::Pick(const Vec2& InCanvasPoint) const {
    for (int32_t i = m_Roots.Size() - 1; i >= 0; i--) {
        UINode* root = m_Roots[i].Get();
        if (!root) {
            continue;
        }
        if (UINode* hit = PickRecursive(root, InCanvasPoint)) {
            if (IsInDesign(hit)) {
                return hit;
            }
        }
    }
    return nullptr;
}

void UILayoutGizmo::CornerPoints(Vec2 OutCorners[4]) const {
    UINode* node = m_Primary.Get();
    if (!node) {
        OutCorners[0] = OutCorners[1] = OutCorners[2] = OutCorners[3] = Vec2(0.0f);
        return;
    }
    const UIRectF& rect = node->GetGeometry();
    const Mat4& world = node->GetWorldMatrix();
    OutCorners[0] = Transform(world, rect.Min());
    OutCorners[1] = Transform(world, Vec2(rect.Max().x, rect.Min().y));
    OutCorners[2] = Transform(world, rect.Max());
    OutCorners[3] = Transform(world, Vec2(rect.Min().x, rect.Max().y));
}

Vec2 UILayoutGizmo::HandlePoint(Handle InHandle) const {
    UINode* node = m_Primary.Get();
    if (!node) {
        return Vec2(0.0f);
    }
    Vec2 corners[4];
    CornerPoints(corners);
    switch (InHandle) {
        case Handle::TopLeft:     return corners[0];
        case Handle::TopRight:    return corners[1];
        case Handle::BottomRight: return corners[2];
        case Handle::BottomLeft:  return corners[3];
        case Handle::Top:         return (corners[0] + corners[1]) * 0.5f;
        case Handle::Right:       return (corners[1] + corners[2]) * 0.5f;
        case Handle::Bottom:      return (corners[2] + corners[3]) * 0.5f;
        case Handle::Left:        return (corners[3] + corners[0]) * 0.5f;
        case Handle::Pivot: {
            const UIRectF& rect = node->GetGeometry();
            return Transform(node->GetWorldMatrix(), rect.Min() + node->Pivot * rect.Size);
        }
        case Handle::Anchor: {
            const UIRectF parent = ParentContentRect(node);
            return Transform(ParentWorldMatrix(node), parent.Min() + node->Anchor * parent.Size);
        }
        default: return (corners[0] + corners[2]) * 0.5f;
    }
}

UILayoutGizmo::Handle UILayoutGizmo::HitTest(const Vec2& InCanvasPoint) const {
    UINode* node = m_Primary.Get();
    if (!IsActive() || !node) {
        return Handle::None;
    }
    const float slop = s_HandleSlop * m_PixelScale;

    if (Tool == UILayoutTool::Rotate) {
        const Vec2 local = Transform(glm::inverse(node->GetWorldMatrix()), InCanvasPoint);
        return node->GetGeometry().Contains(local) ? Handle::Rotate : Handle::None;
    }

    if (glm::length(InCanvasPoint - HandlePoint(Handle::Anchor)) <= slop) {
        return Handle::Anchor;
    }
    if (glm::length(InCanvasPoint - HandlePoint(Handle::Pivot)) <= slop) {
        return Handle::Pivot;
    }

    static const Handle s_Corners[4] = { Handle::TopLeft, Handle::TopRight, Handle::BottomRight, Handle::BottomLeft };
    for (Handle handle : s_Corners) {
        if (glm::length(InCanvasPoint - HandlePoint(handle)) <= slop) {
            return handle;
        }
    }
    static const Handle s_Edges[4] = { Handle::Top, Handle::Right, Handle::Bottom, Handle::Left };
    for (Handle handle : s_Edges) {
        if (glm::length(InCanvasPoint - HandlePoint(handle)) <= slop) {
            return handle;
        }
    }

    const Vec2 local = Transform(glm::inverse(node->GetWorldMatrix()), InCanvasPoint);
    return node->GetGeometry().Contains(local) ? Handle::Body : Handle::None;
}

void UILayoutGizmo::SetHover(const Vec2& InCanvasPoint) {
    if (IsDragging()) {
        return;
    }
    m_Hover = HitTest(InCanvasPoint);
    m_HoverNode = m_Hover == Handle::None ? Pick(InCanvasPoint) : nullptr;
}

void UILayoutGizmo::ClearHover() {
    if (!IsDragging()) {
        m_Hover = Handle::None;
        m_HoverNode = nullptr;
    }
}

CursorIcon UILayoutGizmo::GetCursor() const {
    switch (IsDragging() ? m_DragHandle : m_Hover) {
        case Handle::Left:
        case Handle::Right:       return CursorIcon::ResizeH;
        case Handle::Top:
        case Handle::Bottom:      return CursorIcon::ResizeV;
        case Handle::TopLeft:
        case Handle::BottomRight: return CursorIcon::ResizeNWSE;
        case Handle::TopRight:
        case Handle::BottomLeft:  return CursorIcon::ResizeNESW;
        case Handle::Body:        return CursorIcon::ResizeAll;
        case Handle::Pivot:
        case Handle::Anchor:
        case Handle::Rotate:      return CursorIcon::Hand;
        default:                  return CursorIcon::Arrow;
    }
}

Vec2 UILayoutGizmo::ToNodeLocalDelta(const Vec2& InCanvasDelta) const {
    UINode* node = m_Primary.Get();
    if (!node) {
        return InCanvasDelta;
    }
    const Mat4 inverse = glm::inverse(node->GetWorldMatrix());
    return Transform(inverse, InCanvasDelta) - Transform(inverse, Vec2(0.0f));
}

Vec2 UILayoutGizmo::ToParentLocalDelta(const Vec2& InCanvasDelta) const {
    const Mat4 inverse = glm::inverse(ParentWorldMatrix(m_Primary.Get()));
    return Transform(inverse, InCanvasDelta) - Transform(inverse, Vec2(0.0f));
}

Vec2 UILayoutGizmo::RotateIntoParent(UINode* InNode, const Vec2& InLocalDelta) {
    if (!InNode) {
        return InLocalDelta;
    }
    const Mat4 local = glm::inverse(ParentWorldMatrix(InNode)) * InNode->GetWorldMatrix();
    const Vec2 origin = Transform(local, Vec2(0.0f));
    return (Transform(local, Vec2(InLocalDelta.x, 0.0f)) - origin)
         + (Transform(local, Vec2(0.0f, InLocalDelta.y)) - origin);
}

bool UILayoutGizmo::BeginDrag(const Vec2& InCanvasPoint) {
    const Handle handle = HitTest(InCanvasPoint);
    if (handle == Handle::None) {
        return false;
    }

    m_DragTargets.Clear();
    for (const WeakObjectPtr<UINode>& weak : m_Targets) {
        UINode* node = weak.Get();
        if (!node) {
            continue;
        }
        DragTarget target;
        target.Node = node;
        target.Position = node->Position;
        target.Size = node->Size;
        target.Anchor = node->Anchor;
        target.Pivot = node->Pivot;
        target.Rotation = node->Rotation;
        target.Geometry = node->GetGeometry();
        target.ParentContent = ParentContentRect(node);
        m_DragTargets.Add(target);
    }
    if (m_DragTargets.IsEmpty()) {
        return false;
    }

    m_DragHandle = handle;
    m_Hover = handle;
    m_DragOrigin = InCanvasPoint;
    m_DragGrabOffset = (handle == Handle::Pivot || handle == Handle::Anchor)
        ? HandlePoint(handle) - InCanvasPoint : Vec2(0.0f);
    m_Guides.Clear();

    if (handle == Handle::Rotate) {
        const Vec2 offset = InCanvasPoint - HandlePoint(Handle::Pivot);
        m_DragLastAngle = std::atan2(offset.y, offset.x);
        m_DragAccumulatedAngle = 0.0f;
    }
    return true;
}

void UILayoutGizmo::Drag(const Vec2& InCanvasPoint) {
    if (!IsDragging()) {
        return;
    }
    m_Guides.Clear();
    switch (m_DragHandle) {
        case Handle::Body:   ApplyMove(InCanvasPoint); break;
        case Handle::Pivot:  ApplyPivot(InCanvasPoint); break;
        case Handle::Anchor: ApplyAnchor(InCanvasPoint); break;
        case Handle::Rotate: ApplyRotate(InCanvasPoint); break;
        default:             ApplyResize(InCanvasPoint); break;
    }
}

void UILayoutGizmo::EndDrag() {
    m_DragHandle = Handle::None;
    m_DragTargets.Clear();
    m_Guides.Clear();
}

void UILayoutGizmo::CollectSnapLines(Array<float>& OutVertical, Array<float>& OutHorizontal) const {
    UINode* node = m_Primary.Get();
    if (!node) {
        return;
    }
    const UIRectF parent = ParentContentRect(node);
    OutVertical.Add(parent.Min().x);
    OutVertical.Add(parent.Center().x);
    OutVertical.Add(parent.Max().x);
    OutHorizontal.Add(parent.Min().y);
    OutHorizontal.Add(parent.Center().y);
    OutHorizontal.Add(parent.Max().y);

    Node* parentNode = node->GetParent();
    if (!parentNode) {
        return;
    }
    for (uint32_t i = 0; i < parentNode->GetChildCount(); i++) {
        UINode* sibling = parentNode->GetChild((int)i)->As<UINode>();
        if (!sibling || !sibling->IsEnabled()) {
            continue;
        }
        bool isTarget = false;
        for (const DragTarget& target : m_DragTargets) {
            isTarget = isTarget || target.Node.Get() == sibling;
        }
        if (isTarget) {
            continue;
        }
        const UIRectF& rect = sibling->GetGeometry();
        OutVertical.Add(rect.Min().x);
        OutVertical.Add(rect.Center().x);
        OutVertical.Add(rect.Max().x);
        OutHorizontal.Add(rect.Min().y);
        OutHorizontal.Add(rect.Center().y);
        OutHorizontal.Add(rect.Max().y);
    }
}

Vec2 UILayoutGizmo::SnapRect(const UIRectF& InRect, bool InSnapX, bool InSnapY) {
    Array<float> vertical;
    Array<float> horizontal;
    CollectSnapLines(vertical, horizontal);

    const float threshold = s_SnapThreshold * m_PixelScale;
    const UIRectF parent = ParentContentRect(m_Primary.Get());
    Vec2 correction(0.0f);

    if (InSnapX) {
        const float candidates[3] = { InRect.Min().x, InRect.Center().x, InRect.Max().x };
        float best = threshold;
        for (float line : vertical) {
            for (float candidate : candidates) {
                if (std::abs(line - candidate) < best) {
                    best = std::abs(line - candidate);
                    correction.x = line - candidate;
                    m_Guides.Clear();
                    m_Guides.Add({ Vec2(line, parent.Min().y), Vec2(line, parent.Max().y) });
                }
            }
        }
    }
    if (InSnapY) {
        const float candidates[3] = { InRect.Min().y, InRect.Center().y, InRect.Max().y };
        float best = threshold;
        for (float line : horizontal) {
            for (float candidate : candidates) {
                if (std::abs(line - candidate) < best) {
                    best = std::abs(line - candidate);
                    correction.y = line - candidate;
                    m_Guides.Add({ Vec2(parent.Min().x, line), Vec2(parent.Max().x, line) });
                }
            }
        }
    }
    return correction;
}

void UILayoutGizmo::ApplyMove(const Vec2& InCanvasPoint) {
    Vec2 delta = ToParentLocalDelta(InCanvasPoint - m_DragOrigin);
    if (IsConstraining()) {
        if (std::abs(delta.x) >= std::abs(delta.y)) {
            delta.y = 0.0f;
        } else {
            delta.x = 0.0f;
        }
    }
    if (IsGridSnapping()) {
        delta = Vec2(SnapToStep(delta.x, s_GridStep), SnapToStep(delta.y, s_GridStep));
    } else if (!m_DragTargets.IsEmpty() && !IsRotated(m_Primary.Get())) {
        const UIRectF& start = m_DragTargets.LastItem().Geometry;
        delta += SnapRect(UIRectF(start.Position + delta, start.Size), true, true);
    }

    for (const DragTarget& target : m_DragTargets) {
        UINode* node = target.Node.Get();
        if (!node) {
            continue;
        }
        node->Position.X.Pixels = target.Position.X.Pixels + delta.x;
        node->Position.Y.Pixels = target.Position.Y.Pixels + delta.y;
        node->MarkPropertyOverridden("Position");
    }
}

void UILayoutGizmo::ApplyResize(const Vec2& InCanvasPoint) {
    float signX = 0.0f;
    float signY = 0.0f;
    ResizeSigns(m_DragHandle, signX, signY);
    if (m_DragTargets.IsEmpty()) {
        return;
    }

    const DragTarget& primary = m_DragTargets.LastItem();
    Vec2 delta = ToNodeLocalDelta(InCanvasPoint - m_DragOrigin);
    delta.x = signX != 0.0f ? delta.x : 0.0f;
    delta.y = signY != 0.0f ? delta.y : 0.0f;

    if (IsGridSnapping()) {
        delta = Vec2(SnapToStep(delta.x, s_GridStep), SnapToStep(delta.y, s_GridStep));
    } else if (!IsRotated(m_Primary.Get())) {
        const UIRectF& start = primary.Geometry;
        UIRectF moved = start;
        if (signX > 0.0f) {
            moved.Size.x = start.Size.x + delta.x;
        } else if (signX < 0.0f) {
            moved.Position.x = start.Position.x + delta.x;
            moved.Size.x = start.Size.x - delta.x;
        }
        if (signY > 0.0f) {
            moved.Size.y = start.Size.y + delta.y;
        } else if (signY < 0.0f) {
            moved.Position.y = start.Position.y + delta.y;
            moved.Size.y = start.Size.y - delta.y;
        }
        const Vec2 correction = SnapRect(moved, signX != 0.0f, signY != 0.0f);
        delta += Vec2(signX < 0.0f ? correction.x : correction.x, signY < 0.0f ? correction.y : correction.y);
    }

    if (IsConstraining() && signX != 0.0f && signY != 0.0f) {
        const float width = SafeExtent(primary.Geometry.Size.x);
        const float factor = (width + signX * delta.x) / width;
        delta.y = signY * primary.Geometry.Size.y * (factor - 1.0f);
    }

    const bool relative = IsRelativeEditing();
    for (const DragTarget& target : m_DragTargets) {
        UINode* node = target.Node.Get();
        if (!node) {
            continue;
        }
        const Vec2 parentSize = Vec2(SafeExtent(target.ParentContent.Size.x), SafeExtent(target.ParentContent.Size.y));
        Vec2 compensation(0.0f);

        if (signX != 0.0f) {
            const float grow = signX * delta.x;
            if (glm::max(target.Geometry.Size.x + grow, 0.0f) >= s_MinExtent) {
                if (relative) {
                    node->Size.X.Fraction = target.Size.X.Fraction + grow / parentSize.x;
                } else {
                    node->Size.X.Pixels = target.Size.X.Pixels + grow;
                }
                compensation.x = (signX > 0.0f ? target.Pivot.x : 1.0f - target.Pivot.x) * delta.x;
            }
        }
        if (signY != 0.0f) {
            const float grow = signY * delta.y;
            if (glm::max(target.Geometry.Size.y + grow, 0.0f) >= s_MinExtent) {
                if (relative) {
                    node->Size.Y.Fraction = target.Size.Y.Fraction + grow / parentSize.y;
                } else {
                    node->Size.Y.Pixels = target.Size.Y.Pixels + grow;
                }
                compensation.y = (signY > 0.0f ? target.Pivot.y : 1.0f - target.Pivot.y) * delta.y;
            }
        }

        compensation = RotateIntoParent(node, compensation);
        node->Position.X.Pixels = target.Position.X.Pixels + compensation.x;
        node->Position.Y.Pixels = target.Position.Y.Pixels + compensation.y;
        node->MarkPropertyOverridden("Size");
        node->MarkPropertyOverridden("Position");
    }
}

void UILayoutGizmo::ApplyPivot(const Vec2& InCanvasPoint) {
    UINode* node = m_Primary.Get();
    if (!node || m_DragTargets.IsEmpty()) {
        return;
    }
    const DragTarget& target = m_DragTargets.LastItem();
    const Vec2 local = Transform(glm::inverse(node->GetWorldMatrix()), InCanvasPoint + m_DragGrabOffset);
    const Vec2 size = Vec2(SafeExtent(target.Geometry.Size.x), SafeExtent(target.Geometry.Size.y));

    Vec2 pivot = (local - target.Geometry.Min()) / size;
    for (int32_t axis = 0; axis < 2; axis++) {
        static const float s_Stops[3] = { 0.0f, 0.5f, 1.0f };
        pivot[axis] = SnapToStops(pivot[axis], s_Stops, 3, s_FractionSnapPixels * m_PixelScale / size[axis]);
        pivot[axis] = glm::clamp(pivot[axis], -1.0f, 2.0f);
    }

    node->Pivot = pivot;
    const Vec2 compensation = RotateIntoParent(node, (pivot - target.Pivot) * target.Geometry.Size);
    node->Position.X.Pixels = target.Position.X.Pixels + compensation.x;
    node->Position.Y.Pixels = target.Position.Y.Pixels + compensation.y;
    node->MarkPropertyOverridden("Pivot");
    node->MarkPropertyOverridden("Position");
}

void UILayoutGizmo::ApplyAnchor(const Vec2& InCanvasPoint) {
    UINode* node = m_Primary.Get();
    if (!node || m_DragTargets.IsEmpty()) {
        return;
    }
    const DragTarget& target = m_DragTargets.LastItem();
    const Vec2 local = Transform(glm::inverse(ParentWorldMatrix(node)), InCanvasPoint + m_DragGrabOffset);
    const Vec2 parentSize = Vec2(SafeExtent(target.ParentContent.Size.x), SafeExtent(target.ParentContent.Size.y));
    const Vec2 own = (target.Geometry.Min() - target.ParentContent.Min()) / parentSize;
    const Vec2 ownSize = target.Geometry.Size / parentSize;

    Vec2 anchor = (local - target.ParentContent.Min()) / parentSize;
    for (int32_t axis = 0; axis < 2; axis++) {
        const float stops[6] = { 0.0f, 0.5f, 1.0f, own[axis], own[axis] + ownSize[axis] * 0.5f, own[axis] + ownSize[axis] };
        anchor[axis] = SnapToStops(anchor[axis], stops, 6, s_FractionSnapPixels * m_PixelScale / parentSize[axis]);
        anchor[axis] = glm::clamp(anchor[axis], -1.0f, 2.0f);
    }

    node->Anchor = anchor;
    node->Position.X.Pixels = target.Position.X.Pixels - (anchor.x - target.Anchor.x) * target.ParentContent.Size.x;
    node->Position.Y.Pixels = target.Position.Y.Pixels - (anchor.y - target.Anchor.y) * target.ParentContent.Size.y;
    node->MarkPropertyOverridden("Anchor");
    node->MarkPropertyOverridden("Position");
}

void UILayoutGizmo::ApplyRotate(const Vec2& InCanvasPoint) {
    if (m_DragTargets.IsEmpty()) {
        return;
    }
    const Vec2 offset = InCanvasPoint - HandlePoint(Handle::Pivot);
    if (glm::dot(offset, offset) < 1e-6f) {
        return;
    }
    const float angle = std::atan2(offset.y, offset.x);
    float step = angle - m_DragLastAngle;
    while (step > glm::pi<float>()) {
        step -= glm::two_pi<float>();
    }
    while (step < -glm::pi<float>()) {
        step += glm::two_pi<float>();
    }
    m_DragLastAngle = angle;
    m_DragAccumulatedAngle += step;

    float degrees = glm::degrees(m_DragAccumulatedAngle);
    if (IsGridSnapping()) {
        degrees = SnapToStep(degrees, s_RotateSnapDegrees);
    }

    for (const DragTarget& target : m_DragTargets) {
        if (UINode* node = target.Node.Get()) {
            node->Rotation.z = target.Rotation.z + degrees;
            node->MarkPropertyOverridden("Rotation");
        }
    }
}

void UILayoutGizmo::PaintHandle(UIDrawList& OutDrawList, Handle InHandle, const Vec4& InColor) const {
    const Vec2 center = HandlePoint(InHandle);
    const float half = s_HandleHalf * m_PixelScale;
    const Vec4 color = ((IsDragging() ? m_DragHandle : m_Hover) == InHandle) ? s_HandleHoverColor : InColor;
    OutDrawList.AddRect(UIRectF(center - Vec2(half + m_PixelScale), Vec2((half + m_PixelScale) * 2.0f)), s_OutlineColor);
    OutDrawList.AddRect(UIRectF(center - Vec2(half), Vec2(half * 2.0f)), color);
}

void UILayoutGizmo::PaintRectChrome(UIDrawList& OutDrawList) const {
    UINode* node = m_Primary.Get();
    if (!node) {
        return;
    }

    const UIRectF parent = ParentContentRect(node);
    const Mat4 parentWorld = ParentWorldMatrix(node);
    const Vec2 parentCorners[4] = {
        Transform(parentWorld, parent.Min()),
        Transform(parentWorld, Vec2(parent.Max().x, parent.Min().y)),
        Transform(parentWorld, parent.Max()),
        Transform(parentWorld, Vec2(parent.Min().x, parent.Max().y))
    };
    for (int32_t i = 0; i < 4; i++) {
        OutDrawList.AddLine(parentCorners[i], parentCorners[(i + 1) % 4], s_OutlineWidth * m_PixelScale, s_ParentColor);
    }

    Vec2 corners[4];
    CornerPoints(corners);
    for (int32_t i = 0; i < 4; i++) {
        OutDrawList.AddLine(corners[i], corners[(i + 1) % 4], s_OutlineWidth * 2.0f * m_PixelScale, s_OutlineColor);
    }

    const UIRectF content = node->GetContentRect();
    const UIRectF& geometry = node->GetGeometry();
    if (content.Position != geometry.Position || content.Size != geometry.Size) {
        const Mat4& world = node->GetWorldMatrix();
        const Vec2 padded[4] = {
            Transform(world, content.Min()),
            Transform(world, Vec2(content.Max().x, content.Min().y)),
            Transform(world, content.Max()),
            Transform(world, Vec2(content.Min().x, content.Max().y))
        };
        for (int32_t i = 0; i < 4; i++) {
            OutDrawList.AddLine(padded[i], padded[(i + 1) % 4], s_OutlineWidth * m_PixelScale, s_ParentColor);
        }
    }
}

void UILayoutGizmo::PaintReadout(UIDrawList& OutDrawList) const {
    UINode* node = m_Primary.Get();
    Font* font = UINode::GetDefaultFont();
    if (!node || !font || !IsDragging()) {
        return;
    }

    String text;
    switch (m_DragHandle) {
        case Handle::Body:
            text = FormatNumber(node->Position.X.Pixels) + ", " + FormatNumber(node->Position.Y.Pixels);
            break;
        case Handle::Pivot:
            text = "pivot " + FormatNumber(node->Pivot.x) + ", " + FormatNumber(node->Pivot.y);
            break;
        case Handle::Anchor:
            text = "anchor " + FormatNumber(node->Anchor.x) + ", " + FormatNumber(node->Anchor.y);
            break;
        case Handle::Rotate:
            text = FormatNumber(node->Rotation.z) + " deg";
            break;
        default: {
            const UIRectF& rect = node->GetGeometry();
            text = FormatNumber(rect.Size.x) + " x " + FormatNumber(rect.Size.y);
            if (IsRelativeEditing()) {
                text += "  (" + FormatNumber(node->Size.X.Fraction) + ", " + FormatNumber(node->Size.Y.Fraction) + " rel)";
            }
            break;
        }
    }

    const float height = s_ReadoutSize * m_PixelScale;
    const Vec2 anchor = HandlePoint(Handle::TopLeft) - Vec2(0.0f, height * 1.8f);
    const Vec2 size = Vec2(height * 0.62f * (float)text.size() + height * 0.8f, height * 1.5f);
    OutDrawList.AddRect(UIRectF(anchor, size), s_ReadoutBackground);
    OutDrawList.AddText(font, text, anchor + Vec2(height * 0.4f, height * 0.22f), height, EditorStyle::TextBright);
}

void UILayoutGizmo::Paint(UIDrawList& OutDrawList) const {
    if (UINode* hovered = m_HoverNode.Get()) {
        if (hovered != m_Primary.Get()) {
            const UIRectF& rect = hovered->GetGeometry();
            const Mat4& world = hovered->GetWorldMatrix();
            const Vec2 corners[4] = {
                Transform(world, rect.Min()),
                Transform(world, Vec2(rect.Max().x, rect.Min().y)),
                Transform(world, rect.Max()),
                Transform(world, Vec2(rect.Min().x, rect.Max().y))
            };
            for (int32_t i = 0; i < 4; i++) {
                OutDrawList.AddLine(corners[i], corners[(i + 1) % 4], s_OutlineWidth * m_PixelScale, s_HoverColor);
            }
        }
    }

    UINode* node = m_Primary.Get();
    if (!IsActive() || !node) {
        return;
    }

    PaintRectChrome(OutDrawList);

    for (const SnapGuide& guide : m_Guides) {
        OutDrawList.AddLine(guide.From, guide.To, s_OutlineWidth * m_PixelScale, s_GuideColor);
    }

    const Vec2 anchorPoint = HandlePoint(Handle::Anchor);
    const float arm = s_AnchorArm * m_PixelScale;
    const float thickness = s_OutlineWidth * m_PixelScale;
    const Vec4 anchorColor = ((IsDragging() ? m_DragHandle : m_Hover) == Handle::Anchor) ? s_HandleHoverColor : s_AnchorColor;
    for (int32_t i = 0; i < 4; i++) {
        const Vec2 direction = Vec2(i == 0 ? -1.0f : i == 1 ? 1.0f : 0.0f, i == 2 ? -1.0f : i == 3 ? 1.0f : 0.0f);
        OutDrawList.AddLine(anchorPoint + direction * (arm * 0.35f), anchorPoint + direction * arm, thickness, anchorColor);
    }

    if (Tool == UILayoutTool::Rect) {
        const int32_t stackAxis = StackAxisOf(node);
        static const Handle s_Order[8] = {
            Handle::TopLeft, Handle::Top, Handle::TopRight, Handle::Right,
            Handle::BottomRight, Handle::Bottom, Handle::BottomLeft, Handle::Left
        };
        for (Handle handle : s_Order) {
            float signX = 0.0f;
            float signY = 0.0f;
            ResizeSigns(handle, signX, signY);
            const bool drivenByStack = (stackAxis == 0 && signX != 0.0f && signY == 0.0f)
                                    || (stackAxis == 1 && signY != 0.0f && signX == 0.0f);
            PaintHandle(OutDrawList, handle, drivenByStack ? s_StackHandleColor : s_HandleColor);
        }
    }

    const Vec2 pivotPoint = HandlePoint(Handle::Pivot);
    const Vec4 pivotColor = ((IsDragging() ? m_DragHandle : m_Hover) == Handle::Pivot) ? s_HandleHoverColor : s_PivotColor;
    OutDrawList.AddRing(pivotPoint, s_PivotRadius * m_PixelScale, thickness * 2.0f, pivotColor, 20);

    PaintReadout(OutDrawList);
}
