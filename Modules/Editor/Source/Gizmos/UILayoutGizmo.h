#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "GameFramework/UILayout.h"
#include "InputSystem/CursorIcon.h"
#include "UILayoutGizmo.gen.h"

class MajorTab;
class UICanvas;
class UIDrawList;
class UINode;

enum class UILayoutTool : uint8_t { Select = 0, Rect, Rotate };

class UILayoutGizmo : public Object {
public:
    ARTIFACT_CLASS();

    enum class Handle : uint8_t {
        None = 0, Body,
        Left, Top, Right, Bottom,
        TopLeft, TopRight, BottomRight, BottomLeft,
        Pivot, Anchor, Rotate
    };

    UILayoutTool Tool = UILayoutTool::Rect;

    void Update(MajorTab* InMajorTab, const Array<UINode*>& InRoots, const UIRectF& InRootRect, float InPixelScale);

    bool IsActive() const;
    bool IsDragging() const { return m_DragHandle != Handle::None; }
    bool IsEngaged() const { return IsDragging() || m_Hover != Handle::None; }
    CursorIcon GetCursor() const;

    UINode* Pick(const Vec2& InCanvasPoint) const;
    void SetHover(const Vec2& InCanvasPoint);
    void ClearHover();
    bool BeginDrag(const Vec2& InCanvasPoint);
    void Drag(const Vec2& InCanvasPoint);
    void EndDrag();

    void Paint(UIDrawList& OutDrawList) const;

private:
    struct DragTarget {
        WeakObjectPtr<UINode> Node;
        UIVec2 Position;
        UIVec2 Size;
        Vec2 Anchor = Vec2(0.0f);
        Vec2 Pivot = Vec2(0.0f);
        Vec3 Rotation = Vec3(0.0f);
        UIRectF Geometry;
        UIRectF ParentContent;
    };

    struct SnapGuide {
        Vec2 From = Vec2(0.0f);
        Vec2 To = Vec2(0.0f);
    };

    static Vec2 Transform(const Mat4& InMatrix, const Vec2& InPoint);
    static UINode* ParentNodeOf(UINode* InNode);
    static Mat4 ParentWorldMatrix(UINode* InNode);
    static int32_t StackAxisOf(UINode* InNode);
    UIRectF ParentContentRect(UINode* InNode) const;
    bool IsInDesign(UINode* InNode) const;

    UINode* PickRecursive(UINode* InNode, const Vec2& InCanvasPoint) const;
    void CornerPoints(Vec2 OutCorners[4]) const;
    Vec2 HandlePoint(Handle InHandle) const;
    Handle HitTest(const Vec2& InCanvasPoint) const;

    Vec2 ToNodeLocalDelta(const Vec2& InCanvasDelta) const;
    Vec2 ToParentLocalDelta(const Vec2& InCanvasDelta) const;
    static Vec2 RotateIntoParent(UINode* InNode, const Vec2& InLocalDelta);

    void CollectSnapLines(Array<float>& OutVertical, Array<float>& OutHorizontal) const;
    Vec2 SnapRect(const UIRectF& InRect, bool InSnapX, bool InSnapY);

    void ApplyMove(const Vec2& InCanvasPoint);
    void ApplyResize(const Vec2& InCanvasPoint);
    void ApplyPivot(const Vec2& InCanvasPoint);
    void ApplyAnchor(const Vec2& InCanvasPoint);
    void ApplyRotate(const Vec2& InCanvasPoint);

    void PaintRectChrome(UIDrawList& OutDrawList) const;
    void PaintHandle(UIDrawList& OutDrawList, Handle InHandle, const Vec4& InColor) const;
    void PaintReadout(UIDrawList& OutDrawList) const;

    Array<WeakObjectPtr<UINode>> m_Roots;
    UIRectF m_RootRect;
    Array<WeakObjectPtr<UINode>> m_Targets;
    WeakObjectPtr<UINode> m_Primary;
    WeakObjectPtr<UINode> m_HoverNode;

    float m_PixelScale = 1.0f;
    Handle m_Hover = Handle::None;
    Handle m_DragHandle = Handle::None;
    Vec2 m_DragOrigin = Vec2(0.0f);
    Vec2 m_DragGrabOffset = Vec2(0.0f);
    float m_DragLastAngle = 0.0f;
    float m_DragAccumulatedAngle = 0.0f;
    Array<DragTarget> m_DragTargets;
    Array<SnapGuide> m_Guides;
};
