#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "GizmoGeometry.h"
#include "TransformGizmo.gen.h"

class CameraNode;
class MajorTab;
class Node3D;

enum class GizmoMode : uint8_t { Select = 0, Translate, Rotate, Scale };
enum class GizmoSpace : uint8_t { World = 0, Local };

class TransformGizmo : public Object {
public:
    ARTIFACT_CLASS();

    enum class Handle : uint8_t { None = 0, AxisX, AxisY, AxisZ, PlaneYZ, PlaneXZ, PlaneXY, Screen, Center };

    GizmoMode Mode = GizmoMode::Translate;
    GizmoSpace Space = GizmoSpace::World;

    void Update(MajorTab* InMajorTab, CameraNode* InViewCamera, const Vec2& InViewportSize);

    bool IsActive() const { return m_Active; }
    bool IsDragging() const { return m_DragHandle != Handle::None; }
    bool IsEngaged() const { return IsDragging() || m_Hover != Handle::None; }

    void SetHover(const Vec2& InViewportPixel);
    void ClearHover() { m_Hover = Handle::None; }
    bool BeginDrag(const Vec2& InViewportPixel);
    void Drag(const Vec2& InViewportPixel);
    void EndDrag();

    const GizmoGeometry& BuildGeometry();

private:
    struct DragTarget {
        WeakObjectPtr<Node3D> Node;
        Vec3 StartPosition = Vec3(0.0f);
        Quat StartRotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        Vec3 StartLocalScale = Vec3(1.0f);
    };

    void CollectTargets(MajorTab* InMajorTab);
    void ComputeFrame();

    float WorldPerPixel(const Vec3& InPoint) const;
    Vec3 Axis(int32_t InIndex) const { return m_Basis[InIndex]; }
    Vec3 DirectionToViewer(const Vec3& InPoint) const;
    bool ProjectToPixel(const Vec3& InWorld, Vec2& OutPixel) const;
    void BuildRay(const Vec2& InPixel, Vec3& OutOrigin, Vec3& OutDirection) const;
    void BuildArcRuns(const Vec3& InNormal, float InRadius, Array<Array<Vec3>>& OutRuns) const;

    Handle HitTest(const Vec2& InPixel) const;
    Handle HitTestLinear(const Vec2& InPixel) const;
    Handle HitTestRotate(const Vec2& InPixel) const;
    bool IsAxisUsable(int32_t InIndex) const;
    void PlaneCorners(int32_t InNormalAxis, Vec3 OutCorners[4]) const;
    Vec4 HandleColor(Handle InHandle, const Vec4& InBase) const;

    float AxisParameter(const Vec3& InRayOrigin, const Vec3& InRayDirection, const Vec3& InAxis) const;
    bool PlanePoint(const Vec3& InRayOrigin, const Vec3& InRayDirection, const Vec3& InNormal, Vec3& OutPoint) const;
    float RotationAngle(const Vec3& InRayOrigin, const Vec3& InRayDirection, const Vec2& InPixel) const;

    void ApplyTranslate(const Vec3& InDelta);
    void ApplyRotate(float InAngle);
    void ApplyScale(const Vec3& InFactor);

    void BuildTranslateGeometry();
    void BuildRotateGeometry();
    void BuildScaleGeometry();
    void BuildPlaneHandle(int32_t InNormalAxis, Handle InHandle);

    Array<WeakObjectPtr<Node3D>> m_Targets;
    WeakObjectPtr<Node3D> m_ActiveTarget;

    bool m_Active = false;
    Vec3 m_Pivot = Vec3(0.0f);
    Mat3 m_Basis = Mat3(1.0f);
    float m_HandleScale = 1.0f;
    Handle m_Hover = Handle::None;

    Mat4 m_ViewProjection = Mat4(1.0f);
    Vec2 m_ViewportSize = Vec2(1.0f);
    Vec3 m_EyePosition = Vec3(0.0f);
    Vec3 m_ViewDirection = Vec3(0.0f, 0.0f, 1.0f);
    Vec3 m_ViewerFromPivot = Vec3(0.0f, 0.0f, -1.0f);
    bool m_Orthographic = false;
    float m_TanHalfFov = 1.0f;
    float m_OrthographicSize = 10.0f;

    Handle m_DragHandle = Handle::None;
    Array<DragTarget> m_DragTargets;
    Vec3 m_DragPivot = Vec3(0.0f);
    Vec3 m_DragAxis = Vec3(1.0f, 0.0f, 0.0f);
    Vec3 m_DragPlaneNormal = Vec3(0.0f, 1.0f, 0.0f);
    Vec3 m_DragStartPoint = Vec3(0.0f);
    float m_DragStartParameter = 0.0f;
    float m_DragStartPixelDistance = 1.0f;
    bool m_DragUsesPlaneAngle = true;
    float m_DragLastAngle = 0.0f;
    float m_DragAccumulatedAngle = 0.0f;

    GizmoGeometry m_Geometry;
};
