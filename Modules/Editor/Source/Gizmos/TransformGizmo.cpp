#include "TransformGizmo.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorStyle.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/Node3D.h"
#include "InputSystem/KeyboardDevice.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

static constexpr float s_AxisLength = 78.0f;
static constexpr float s_ShaftHalfWidth = 1.7f;
static constexpr float s_ArrowLength = 20.0f;
static constexpr float s_ArrowRadius = 6.5f;
static constexpr float s_ScaleBoxHalf = 5.0f;
static constexpr float s_PlaneInner = 26.0f;
static constexpr float s_PlaneOuter = 48.0f;
static constexpr float s_CenterRadius = 7.0f;
static constexpr float s_RingRadius = 84.0f;
static constexpr float s_ScreenRingRadius = 104.0f;
static constexpr float s_RingHalfWidth = 2.0f;
static constexpr float s_AxisPickSlop = 8.0f;
static constexpr float s_RingPickSlop = 7.0f;

static constexpr int32_t s_ArcSamples = 96;
static constexpr float s_AxisFacingLimit = 0.985f;
static constexpr float s_ArcVisibilityBias = -0.02f;

static constexpr float s_TranslateSnap = 0.25f;
static constexpr float s_RotateSnapDegrees = 15.0f;
static constexpr float s_ScaleSnap = 0.1f;
static constexpr float s_MinScaleFactor = 0.001f;

static const Vec4 s_HighlightColor = HexColor(0xFFC02E);
static const Vec4 s_ScreenColor = HexColor(0xD0D4D8);
static const Vec4 s_PlaneFillAlpha = Vec4(1.0f, 1.0f, 1.0f, 0.35f);

static bool IsSnapping() {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && (keyboard->IsPressed(KeyCode::LeftControl) || keyboard->IsPressed(KeyCode::RightControl));
}

static float SnapTo(float InValue, float InStep) {
    return std::round(InValue / InStep) * InStep;
}

static float DistanceToSegment(const Vec2& InPoint, const Vec2& InA, const Vec2& InB) {
    const Vec2 edge = InB - InA;
    const float lengthSquared = glm::dot(edge, edge);
    if (lengthSquared < 1e-6f) {
        return glm::length(InPoint - InA);
    }
    const float t = glm::clamp(glm::dot(InPoint - InA, edge) / lengthSquared, 0.0f, 1.0f);
    return glm::length(InPoint - (InA + edge * t));
}

static bool IsPointInTriangle(const Vec2& InPoint, const Vec2& InA, const Vec2& InB, const Vec2& InC) {
    const auto edgeSign = [](const Vec2& InP, const Vec2& InQ, const Vec2& InR) {
        return (InP.x - InR.x) * (InQ.y - InR.y) - (InQ.x - InR.x) * (InP.y - InR.y);
    };
    const float a = edgeSign(InPoint, InA, InB);
    const float b = edgeSign(InPoint, InB, InC);
    const float c = edgeSign(InPoint, InC, InA);
    return !((a < 0.0f || b < 0.0f || c < 0.0f) && (a > 0.0f || b > 0.0f || c > 0.0f));
}

void TransformGizmo::Update(MajorTab* InMajorTab, CameraNode* InViewCamera, const Vec2& InViewportSize) {
    m_ViewportSize = Vec2(glm::max(InViewportSize.x, 1.0f), glm::max(InViewportSize.y, 1.0f));

    if (!InViewCamera) {
        m_Active = false;
        EndDrag();
        return;
    }

    m_ViewProjection = InViewCamera->GetViewProjectionMatrix();
    m_EyePosition = InViewCamera->GetPosition();
    m_ViewDirection = InViewCamera->GetForwardVector();
    m_Orthographic = InViewCamera->GetProjectionType() == CameraNode::ProjectionType::Orthographic;
    m_TanHalfFov = std::tan(glm::radians(InViewCamera->GetPerspectiveVerticalFOV()) * 0.5f);
    m_OrthographicSize = InViewCamera->GetOrthographicSize();

    if (IsDragging()) {
        m_ViewerFromPivot = DirectionToViewer(m_Pivot);
        m_HandleScale = WorldPerPixel(m_Pivot);
        return;
    }

    CollectTargets(InMajorTab);
    m_Active = Mode != GizmoMode::Select && !m_Targets.IsEmpty();
    if (!m_Active) {
        m_Hover = Handle::None;
        return;
    }
    ComputeFrame();
}

void TransformGizmo::CollectTargets(MajorTab* InMajorTab) {
    m_Targets.Clear();
    m_ActiveTarget = nullptr;
    if (!InMajorTab) {
        return;
    }

    Array<Node3D*> selected;
    for (Object* object : InMajorTab->GetSelection()) {
        if (Node3D* node = Cast<Node3D>(object)) {
            selected.Add(node);
        }
    }

    for (Node3D* node : selected) {
        bool hasSelectedAncestor = false;
        for (Node3D* other : selected) {
            if (other != node && node->IsChildOf(other)) {
                hasSelectedAncestor = true;
                break;
            }
        }
        if (!hasSelectedAncestor) {
            m_Targets.Add(WeakObjectPtr<Node3D>(node));
        }
    }
    if (!m_Targets.IsEmpty()) {
        m_ActiveTarget = m_Targets.LastItem();
    }
}

void TransformGizmo::ComputeFrame() {
    Vec3 sum(0.0f);
    int32_t count = 0;
    for (const WeakObjectPtr<Node3D>& weak : m_Targets) {
        if (Node3D* node = weak.Get()) {
            sum += node->GetPosition();
            count++;
        }
    }
    m_Pivot = count > 0 ? sum / (float)count : Vec3(0.0f);

    Node3D* active = m_ActiveTarget.Get();
    const bool useLocalAxes = Mode == GizmoMode::Scale || Space == GizmoSpace::Local;
    if (useLocalAxes && active) {
        const Mat3 rotation = glm::mat3_cast(active->GetRotation());
        m_Basis = Mat3(glm::normalize(rotation[0]), glm::normalize(rotation[1]), glm::normalize(rotation[2]));
    } else {
        m_Basis = Mat3(1.0f);
    }

    m_HandleScale = WorldPerPixel(m_Pivot);
    m_ViewerFromPivot = DirectionToViewer(m_Pivot);
}

float TransformGizmo::WorldPerPixel(const Vec3& InPoint) const {
    if (m_Orthographic) {
        return m_OrthographicSize / m_ViewportSize.y;
    }
    const float depth = glm::max(glm::dot(InPoint - m_EyePosition, m_ViewDirection), 0.01f);
    return 2.0f * depth * m_TanHalfFov / m_ViewportSize.y;
}

Vec3 TransformGizmo::DirectionToViewer(const Vec3& InPoint) const {
    if (m_Orthographic) {
        return -m_ViewDirection;
    }
    const Vec3 offset = m_EyePosition - InPoint;
    const float length = glm::length(offset);
    return length > 1e-6f ? offset / length : -m_ViewDirection;
}

bool TransformGizmo::ProjectToPixel(const Vec3& InWorld, Vec2& OutPixel) const {
    const Vec4 clip = m_ViewProjection * Vec4(InWorld, 1.0f);
    if (clip.w <= 1e-5f) {
        return false;
    }
    const Vec2 ndc = Vec2(clip) / clip.w;
    OutPixel = Vec2((ndc.x * 0.5f + 0.5f) * m_ViewportSize.x, (ndc.y * 0.5f + 0.5f) * m_ViewportSize.y);
    return true;
}

void TransformGizmo::BuildRay(const Vec2& InPixel, Vec3& OutOrigin, Vec3& OutDirection) const {
    const Vec2 ndc = Vec2(InPixel.x / m_ViewportSize.x, InPixel.y / m_ViewportSize.y) * 2.0f - 1.0f;
    const Mat4 inverseViewProjection = glm::inverse(m_ViewProjection);
    const Vec4 nearPoint = inverseViewProjection * Vec4(ndc.x, ndc.y, 0.0f, 1.0f);
    const Vec4 farPoint = inverseViewProjection * Vec4(ndc.x, ndc.y, 1.0f, 1.0f);
    OutOrigin = Vec3(nearPoint) / nearPoint.w;
    OutDirection = glm::normalize(Vec3(farPoint) / farPoint.w - OutOrigin);
}

bool TransformGizmo::IsAxisUsable(int32_t InIndex) const {
    return std::abs(glm::dot(Axis(InIndex), m_ViewerFromPivot)) < s_AxisFacingLimit;
}

void TransformGizmo::PlaneCorners(int32_t InNormalAxis, Vec3 OutCorners[4]) const {
    const Vec3 first = Axis((InNormalAxis + 1) % 3) * (m_HandleScale);
    const Vec3 second = Axis((InNormalAxis + 2) % 3) * (m_HandleScale);
    OutCorners[0] = m_Pivot + first * s_PlaneInner + second * s_PlaneInner;
    OutCorners[1] = m_Pivot + first * s_PlaneOuter + second * s_PlaneInner;
    OutCorners[2] = m_Pivot + first * s_PlaneOuter + second * s_PlaneOuter;
    OutCorners[3] = m_Pivot + first * s_PlaneInner + second * s_PlaneOuter;
}

void TransformGizmo::BuildArcRuns(const Vec3& InNormal, float InRadius, Array<Array<Vec3>>& OutRuns) const {
    Vec3 axisU, axisV;
    GizmoGeometry::BuildPlaneBasis(InNormal, axisU, axisV);

    Array<Vec3> points;
    Array<uint8_t> visible;
    int32_t visibleCount = 0;
    for (int32_t i = 0; i < s_ArcSamples; i++) {
        const float angle = glm::two_pi<float>() * (float)i / (float)s_ArcSamples;
        const Vec3 radial = axisU * std::cos(angle) + axisV * std::sin(angle);
        points.Add(m_Pivot + radial * InRadius);
        const bool isVisible = glm::dot(radial, m_ViewerFromPivot) > s_ArcVisibilityBias;
        visible.Add(isVisible ? 1 : 0);
        visibleCount += isVisible ? 1 : 0;
    }

    if (visibleCount == s_ArcSamples) {
        OutRuns.Add(points);
        return;
    }

    int32_t start = -1;
    for (int32_t i = 0; i < s_ArcSamples; i++) {
        if (visible[i] != 0 && visible[(i + s_ArcSamples - 1) % s_ArcSamples] == 0) {
            start = i;
            break;
        }
    }
    if (start < 0) {
        return;
    }

    Array<Vec3> run;
    for (int32_t step = 0; step <= s_ArcSamples; step++) {
        const int32_t index = (start + step) % s_ArcSamples;
        if (step < s_ArcSamples && visible[index] != 0) {
            run.Add(points[index]);
            continue;
        }
        if (run.Size() >= 2) {
            OutRuns.Add(run);
        }
        run.Clear();
    }
}

TransformGizmo::Handle TransformGizmo::HitTest(const Vec2& InPixel) const {
    if (!m_Active) {
        return Handle::None;
    }
    switch (Mode) {
        case GizmoMode::Translate:
        case GizmoMode::Scale:
            return HitTestLinear(InPixel);
        case GizmoMode::Rotate:
            return HitTestRotate(InPixel);
        default:
            return Handle::None;
    }
}

TransformGizmo::Handle TransformGizmo::HitTestLinear(const Vec2& InPixel) const {
    Vec2 pivotPixel;
    if (!ProjectToPixel(m_Pivot, pivotPixel)) {
        return Handle::None;
    }

    if (glm::length(InPixel - pivotPixel) <= s_CenterRadius + 3.0f) {
        return Handle::Center;
    }

    static const Handle s_PlaneHandles[3] = { Handle::PlaneYZ, Handle::PlaneXZ, Handle::PlaneXY };
    for (int32_t axis = 0; axis < 3; axis++) {
        if (!IsAxisUsable((axis + 1) % 3) || !IsAxisUsable((axis + 2) % 3)) {
            continue;
        }
        Vec3 corners[4];
        PlaneCorners(axis, corners);
        Vec2 pixels[4];
        bool projected = true;
        for (int32_t i = 0; i < 4; i++) {
            projected = projected && ProjectToPixel(corners[i], pixels[i]);
        }
        if (projected && (IsPointInTriangle(InPixel, pixels[0], pixels[1], pixels[2])
                       || IsPointInTriangle(InPixel, pixels[0], pixels[2], pixels[3]))) {
            return s_PlaneHandles[axis];
        }
    }

    static const Handle s_AxisHandles[3] = { Handle::AxisX, Handle::AxisY, Handle::AxisZ };
    const float reach = (Mode == GizmoMode::Scale ? s_AxisLength + s_ScaleBoxHalf : s_AxisLength + s_ArrowLength) * m_HandleScale;
    Handle best = Handle::None;
    float bestDistance = s_AxisPickSlop;
    for (int32_t axis = 0; axis < 3; axis++) {
        Vec2 tipPixel;
        if (!IsAxisUsable(axis) || !ProjectToPixel(m_Pivot + Axis(axis) * reach, tipPixel)) {
            continue;
        }
        const float distance = DistanceToSegment(InPixel, pivotPixel, tipPixel);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = s_AxisHandles[axis];
        }
    }
    return best;
}

TransformGizmo::Handle TransformGizmo::HitTestRotate(const Vec2& InPixel) const {
    static const Handle s_AxisHandles[3] = { Handle::AxisX, Handle::AxisY, Handle::AxisZ };
    Handle best = Handle::None;
    float bestDistance = s_RingPickSlop;

    const auto testRuns = [&](const Array<Array<Vec3>>& InRuns, Handle InHandle) {
        for (const Array<Vec3>& run : InRuns) {
            Vec2 previous;
            bool hasPrevious = false;
            for (const Vec3& point : run) {
                Vec2 pixel;
                if (!ProjectToPixel(point, pixel)) {
                    hasPrevious = false;
                    continue;
                }
                if (hasPrevious) {
                    const float distance = DistanceToSegment(InPixel, previous, pixel);
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        best = InHandle;
                    }
                }
                previous = pixel;
                hasPrevious = true;
            }
        }
    };

    for (int32_t axis = 0; axis < 3; axis++) {
        Array<Array<Vec3>> runs;
        BuildArcRuns(Axis(axis), s_RingRadius * m_HandleScale, runs);
        testRuns(runs, s_AxisHandles[axis]);
    }

    Array<Array<Vec3>> screenRuns;
    BuildArcRuns(m_ViewerFromPivot, s_ScreenRingRadius * m_HandleScale, screenRuns);
    testRuns(screenRuns, Handle::Screen);

    return best;
}

void TransformGizmo::SetHover(const Vec2& InViewportPixel) {
    if (!IsDragging()) {
        m_Hover = HitTest(InViewportPixel);
    }
}

bool TransformGizmo::BeginDrag(const Vec2& InViewportPixel) {
    if (!m_Active) {
        return false;
    }
    const Handle handle = HitTest(InViewportPixel);
    if (handle == Handle::None) {
        return false;
    }

    m_DragTargets.Clear();
    for (const WeakObjectPtr<Node3D>& weak : m_Targets) {
        Node3D* node = weak.Get();
        if (!node) {
            continue;
        }
        DragTarget target;
        target.Node = node;
        target.StartPosition = node->GetPosition();
        target.StartRotation = node->GetRotation();
        target.StartLocalScale = node->GetLocalScale();
        m_DragTargets.Add(target);
    }
    if (m_DragTargets.IsEmpty()) {
        return false;
    }

    m_DragHandle = handle;
    m_Hover = handle;
    m_DragPivot = m_Pivot;
    m_DragHandleScale = m_HandleScale;

    Vec3 rayOrigin, rayDirection;
    BuildRay(InViewportPixel, rayOrigin, rayDirection);

    switch (handle) {
        case Handle::AxisX:
        case Handle::AxisY:
        case Handle::AxisZ: {
            const int32_t axis = (int32_t)handle - (int32_t)Handle::AxisX;
            m_DragAxis = Axis(axis);
            if (Mode == GizmoMode::Rotate) {
                m_DragPlaneNormal = m_DragAxis;
            } else {
                m_DragStartParameter = AxisParameter(rayOrigin, rayDirection, m_DragAxis);
            }
            break;
        }
        case Handle::PlaneYZ:
        case Handle::PlaneXZ:
        case Handle::PlaneXY: {
            const int32_t axis = (int32_t)handle - (int32_t)Handle::PlaneYZ;
            m_DragPlaneNormal = Axis(axis);
            break;
        }
        case Handle::Screen:
            m_DragAxis = m_ViewerFromPivot;
            m_DragPlaneNormal = m_ViewerFromPivot;
            break;
        case Handle::Center:
            m_DragPlaneNormal = m_ViewerFromPivot;
            break;
        default:
            break;
    }

    if (Mode == GizmoMode::Rotate) {
        m_DragUsesPlaneAngle = std::abs(glm::dot(rayDirection, m_DragPlaneNormal)) > 0.08f;
        m_DragLastAngle = RotationAngle(rayOrigin, rayDirection, InViewportPixel);
        m_DragAccumulatedAngle = 0.0f;
    } else if (handle == Handle::Center) {
        Vec2 pivotPixel;
        m_DragStartPixelDistance = ProjectToPixel(m_Pivot, pivotPixel)
            ? glm::max(glm::length(InViewportPixel - pivotPixel), s_CenterRadius) : s_CenterRadius;
        PlanePoint(rayOrigin, rayDirection, m_DragPlaneNormal, m_DragStartPoint);
    } else if (handle >= Handle::PlaneYZ && handle <= Handle::PlaneXY) {
        PlanePoint(rayOrigin, rayDirection, m_DragPlaneNormal, m_DragStartPoint);
    }
    return true;
}

void TransformGizmo::Drag(const Vec2& InViewportPixel) {
    if (!IsDragging()) {
        return;
    }

    Vec3 rayOrigin, rayDirection;
    BuildRay(InViewportPixel, rayOrigin, rayDirection);
    const bool snap = IsSnapping();

    if (Mode == GizmoMode::Rotate) {
        const float angle = RotationAngle(rayOrigin, rayDirection, InViewportPixel);
        float step = angle - m_DragLastAngle;
        while (step > glm::pi<float>()) {
            step -= glm::two_pi<float>();
        }
        while (step < -glm::pi<float>()) {
            step += glm::two_pi<float>();
        }
        m_DragLastAngle = angle;
        m_DragAccumulatedAngle += step;

        float applied = m_DragAccumulatedAngle;
        if (snap) {
            applied = glm::radians(SnapTo(glm::degrees(applied), s_RotateSnapDegrees));
        }
        ApplyRotate(applied);
        return;
    }

    const int32_t axisIndex = (m_DragHandle >= Handle::AxisX && m_DragHandle <= Handle::AxisZ)
        ? (int32_t)m_DragHandle - (int32_t)Handle::AxisX : -1;
    const int32_t planeIndex = (m_DragHandle >= Handle::PlaneYZ && m_DragHandle <= Handle::PlaneXY)
        ? (int32_t)m_DragHandle - (int32_t)Handle::PlaneYZ : -1;

    if (Mode == GizmoMode::Translate) {
        Vec3 delta(0.0f);
        if (axisIndex >= 0) {
            float offset = AxisParameter(rayOrigin, rayDirection, m_DragAxis) - m_DragStartParameter;
            if (snap) {
                offset = SnapTo(offset, s_TranslateSnap);
            }
            delta = m_DragAxis * offset;
        } else {
            Vec3 point;
            if (!PlanePoint(rayOrigin, rayDirection, m_DragPlaneNormal, point)) {
                return;
            }
            delta = point - m_DragStartPoint;
            if (snap) {
                const Vec3 snapped(SnapTo(glm::dot(delta, Axis(0)), s_TranslateSnap),
                                   SnapTo(glm::dot(delta, Axis(1)), s_TranslateSnap),
                                   SnapTo(glm::dot(delta, Axis(2)), s_TranslateSnap));
                delta = Axis(0) * snapped.x + Axis(1) * snapped.y + Axis(2) * snapped.z;
            }
        }
        ApplyTranslate(delta);
        return;
    }

    const float handleLength = glm::max(s_AxisLength * m_DragHandleScale, 1e-5f);
    Vec3 factor(1.0f);
    if (axisIndex >= 0) {
        float amount = 1.0f + (AxisParameter(rayOrigin, rayDirection, m_DragAxis) - m_DragStartParameter) / handleLength;
        if (snap) {
            amount = SnapTo(amount, s_ScaleSnap);
        }
        factor[axisIndex] = amount;
    } else if (planeIndex >= 0) {
        Vec3 point;
        if (!PlanePoint(rayOrigin, rayDirection, m_DragPlaneNormal, point)) {
            return;
        }
        const Vec3 offset = point - m_DragStartPoint;
        const int32_t firstAxis = (planeIndex + 1) % 3;
        const int32_t secondAxis = (planeIndex + 2) % 3;
        float amount = 1.0f + (glm::dot(offset, Axis(firstAxis)) + glm::dot(offset, Axis(secondAxis))) / (2.0f * handleLength);
        if (snap) {
            amount = SnapTo(amount, s_ScaleSnap);
        }
        factor[firstAxis] = amount;
        factor[secondAxis] = amount;
    } else {
        Vec2 pivotPixel;
        if (!ProjectToPixel(m_DragPivot, pivotPixel)) {
            return;
        }
        float amount = glm::length(InViewportPixel - pivotPixel) / m_DragStartPixelDistance;
        if (snap) {
            amount = SnapTo(amount, s_ScaleSnap);
        }
        factor = Vec3(amount);
    }
    ApplyScale(factor);
}

void TransformGizmo::EndDrag() {
    m_DragHandle = Handle::None;
    m_DragTargets.Clear();
}

float TransformGizmo::AxisParameter(const Vec3& InRayOrigin, const Vec3& InRayDirection, const Vec3& InAxis) const {
    const Vec3 between = m_DragPivot - InRayOrigin;
    const float axisDotRay = glm::dot(InAxis, InRayDirection);
    const float denominator = 1.0f - axisDotRay * axisDotRay;
    if (denominator < 1e-6f) {
        return m_DragStartParameter;
    }
    return (axisDotRay * glm::dot(between, InRayDirection) - glm::dot(between, InAxis)) / denominator;
}

bool TransformGizmo::PlanePoint(const Vec3& InRayOrigin, const Vec3& InRayDirection, const Vec3& InNormal, Vec3& OutPoint) const {
    const float denominator = glm::dot(InRayDirection, InNormal);
    if (std::abs(denominator) < 1e-5f) {
        return false;
    }
    const float distance = glm::dot(m_DragPivot - InRayOrigin, InNormal) / denominator;
    if (distance <= 0.0f) {
        return false;
    }
    OutPoint = InRayOrigin + InRayDirection * distance;
    return true;
}

float TransformGizmo::RotationAngle(const Vec3& InRayOrigin, const Vec3& InRayDirection, const Vec2& InPixel) const {
    Vec3 axisU, axisV;
    GizmoGeometry::BuildPlaneBasis(m_DragPlaneNormal, axisU, axisV);

    Vec3 point;
    if (m_DragUsesPlaneAngle && PlanePoint(InRayOrigin, InRayDirection, m_DragPlaneNormal, point)) {
        const Vec3 radial = point - m_DragPivot;
        return std::atan2(glm::dot(radial, axisV), glm::dot(radial, axisU));
    }

    Vec2 pivotPixel;
    if (!ProjectToPixel(m_DragPivot, pivotPixel)) {
        return m_DragLastAngle;
    }
    const Vec2 offset = InPixel - pivotPixel;
    const float angle = std::atan2(-offset.y, offset.x);
    return glm::dot(m_DragPlaneNormal, m_ViewerFromPivot) < 0.0f ? -angle : angle;
}

void TransformGizmo::ApplyTranslate(const Vec3& InDelta) {
    for (const DragTarget& target : m_DragTargets) {
        if (Node3D* node = target.Node.Get()) {
            node->SetPosition(target.StartPosition + InDelta);
            node->MarkPropertyOverridden("m_LocalPosition");
        }
    }
    m_Pivot = m_DragPivot + InDelta;
}

void TransformGizmo::ApplyRotate(float InAngle) {
    const Quat rotation = glm::angleAxis(InAngle, m_DragPlaneNormal);
    for (const DragTarget& target : m_DragTargets) {
        Node3D* node = target.Node.Get();
        if (!node) {
            continue;
        }
        node->SetRotation(rotation * target.StartRotation);
        node->MarkPropertyOverridden("m_LocalRotation");

        const Vec3 offset = target.StartPosition - m_DragPivot;
        if (glm::dot(offset, offset) > 1e-10f) {
            node->SetPosition(m_DragPivot + rotation * offset);
            node->MarkPropertyOverridden("m_LocalPosition");
        }
    }
}

void TransformGizmo::ApplyScale(const Vec3& InFactor) {
    Vec3 factor = InFactor;
    for (int32_t axis = 0; axis < 3; axis++) {
        factor[axis] = glm::max(factor[axis], s_MinScaleFactor);
    }

    for (const DragTarget& target : m_DragTargets) {
        Node3D* node = target.Node.Get();
        if (!node) {
            continue;
        }
        node->SetLocalScale(target.StartLocalScale * factor);
        node->MarkPropertyOverridden("m_LocalScale");

        const Vec3 offset = target.StartPosition - m_DragPivot;
        if (glm::dot(offset, offset) > 1e-10f) {
            const Vec3 scaled = Axis(0) * (glm::dot(offset, Axis(0)) * factor.x)
                              + Axis(1) * (glm::dot(offset, Axis(1)) * factor.y)
                              + Axis(2) * (glm::dot(offset, Axis(2)) * factor.z);
            node->SetPosition(m_DragPivot + scaled);
            node->MarkPropertyOverridden("m_LocalPosition");
        }
    }
}

Vec4 TransformGizmo::HandleColor(Handle InHandle, const Vec4& InBase) const {
    const Handle highlighted = IsDragging() ? m_DragHandle : m_Hover;
    return InHandle == highlighted ? Vec4(Vec3(s_HighlightColor), InBase.a) : InBase;
}

const GizmoGeometry& TransformGizmo::BuildGeometry() {
    m_Geometry.Clear();
    if (!m_Active) {
        return m_Geometry;
    }
    m_Geometry.SetViewer(m_EyePosition, m_ViewDirection, m_Orthographic);

    switch (Mode) {
        case GizmoMode::Translate: BuildTranslateGeometry(); break;
        case GizmoMode::Rotate:    BuildRotateGeometry();    break;
        case GizmoMode::Scale:     BuildScaleGeometry();     break;
        default: break;
    }

    m_Geometry.SortForPainter();
    return m_Geometry;
}

static const Vec4& AxisColor(int32_t InIndex) {
    static const Vec4 s_Colors[3] = { EditorStyle::TransformX, EditorStyle::TransformY, EditorStyle::TransformZ };
    return s_Colors[InIndex];
}

void TransformGizmo::BuildPlaneHandle(int32_t InNormalAxis, Handle InHandle) {
    if (!IsAxisUsable((InNormalAxis + 1) % 3) || !IsAxisUsable((InNormalAxis + 2) % 3)) {
        return;
    }
    Vec3 corners[4];
    PlaneCorners(InNormalAxis, corners);
    const Vec4 color = HandleColor(InHandle, AxisColor(InNormalAxis));
    m_Geometry.AddQuad(corners[0], corners[1], corners[2], corners[3], color * s_PlaneFillAlpha);
    m_Geometry.AddRibbon({ corners[0], corners[1], corners[2], corners[3] }, true,
                         s_ShaftHalfWidth * m_HandleScale, color);
}

void TransformGizmo::BuildTranslateGeometry() {
    const float shaftLength = s_AxisLength * m_HandleScale;
    const float arrowLength = s_ArrowLength * m_HandleScale;

    for (int32_t axis = 0; axis < 3; axis++) {
        if (!IsAxisUsable(axis)) {
            continue;
        }
        const Handle handle = (Handle)((int32_t)Handle::AxisX + axis);
        const Vec4 color = HandleColor(handle, AxisColor(axis));
        const Vec3 direction = Axis(axis);
        m_Geometry.AddLine(m_Pivot, m_Pivot + direction * shaftLength, s_ShaftHalfWidth * m_HandleScale, color);
        m_Geometry.AddCone(m_Pivot + direction * shaftLength, m_Pivot + direction * (shaftLength + arrowLength),
                           s_ArrowRadius * m_HandleScale, color);
    }

    BuildPlaneHandle(0, Handle::PlaneYZ);
    BuildPlaneHandle(1, Handle::PlaneXZ);
    BuildPlaneHandle(2, Handle::PlaneXY);

    m_Geometry.AddDisc(m_Pivot, s_CenterRadius * m_HandleScale, HandleColor(Handle::Center, s_ScreenColor));
}

void TransformGizmo::BuildRotateGeometry() {
    const float ribbonWidth = s_RingHalfWidth * m_HandleScale;

    for (int32_t axis = 0; axis < 3; axis++) {
        const Handle handle = (Handle)((int32_t)Handle::AxisX + axis);
        const Vec4 color = HandleColor(handle, AxisColor(axis));
        Array<Array<Vec3>> runs;
        BuildArcRuns(Axis(axis), s_RingRadius * m_HandleScale, runs);
        for (const Array<Vec3>& run : runs) {
            m_Geometry.AddRibbon(run, false, ribbonWidth, color);
        }
    }

    Array<Array<Vec3>> screenRuns;
    BuildArcRuns(m_ViewerFromPivot, s_ScreenRingRadius * m_HandleScale, screenRuns);
    const Vec4 screenColor = HandleColor(Handle::Screen, s_ScreenColor);
    for (const Array<Vec3>& run : screenRuns) {
        m_Geometry.AddRibbon(run, false, ribbonWidth, screenColor);
    }
}

void TransformGizmo::BuildScaleGeometry() {
    const float shaftLength = s_AxisLength * m_HandleScale;
    const float boxHalf = s_ScaleBoxHalf * m_HandleScale;

    for (int32_t axis = 0; axis < 3; axis++) {
        if (!IsAxisUsable(axis)) {
            continue;
        }
        const Handle handle = (Handle)((int32_t)Handle::AxisX + axis);
        const Vec4 color = HandleColor(handle, AxisColor(axis));
        const Vec3 direction = Axis(axis);
        const Vec3 tip = m_Pivot + direction * shaftLength;
        m_Geometry.AddLine(m_Pivot, tip, s_ShaftHalfWidth * m_HandleScale, color);
        m_Geometry.AddBox(tip, Axis(0) * boxHalf, Axis(1) * boxHalf, Axis(2) * boxHalf, color);
    }

    BuildPlaneHandle(0, Handle::PlaneYZ);
    BuildPlaneHandle(1, Handle::PlaneXZ);
    BuildPlaneHandle(2, Handle::PlaneXY);

    const float centerHalf = s_CenterRadius * m_HandleScale;
    m_Geometry.AddBox(m_Pivot, Axis(0) * centerHalf, Axis(1) * centerHalf, Axis(2) * centerHalf,
                      HandleColor(Handle::Center, s_ScreenColor));
}
