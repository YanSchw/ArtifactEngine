#include "CameraNode.h"
#include "Core/Log.h"
#include "World.h"
#include "glm/gtc/matrix_transform.hpp"

CameraNode::CameraNode() {
    RecalculateProjection();
}

void CameraNode::BeginPlay() {
    Super::BeginPlay();
    if (GetWorld()->GetMainCamera() == nullptr) {
        GetWorld()->SetMainCamera(this);
    }
    RecalculateProjection();
}

void CameraNode::TickTransform(bool inWorldSpace) {
    Super::TickTransform(inWorldSpace);
    m_ViewProjection = GetProjectionMatrix() * glm::inverse(GetTransformMatrix());
}

void CameraNode::SetPerspective(float verticalFOV, float nearClip, float farClip) {
    m_ProjectionType = ProjectionType::Perspective;
    m_PerspectiveFOV = verticalFOV;
    m_PerspectiveNear = nearClip;
    m_PerspectiveFar = farClip;
    RecalculateProjection();
}

void CameraNode::SetOrthographic(float size, float nearClip, float farClip) {
    m_ProjectionType = ProjectionType::Orthographic;
    m_OrthographicSize = size;
    m_OrthographicNear = nearClip;
    m_OrthographicFar = farClip;
    RecalculateProjection();
}

void CameraNode::SetViewportSize(uint32_t width, uint32_t height) {
    m_AspectRatio = (float)width / (float)height;
    RecalculateProjection();
}

Vec3 CameraNode::ScreenPosToWorldDirection(const Vec2& InScreenPos, float InWindowWidth, float InWindowHeight) const {
    const Vec2 ndc = Vec2(InScreenPos.x / InWindowWidth, InScreenPos.y / InWindowHeight) * 2.0f - 1.0f;
    const Mat4 inverseViewProjection = glm::inverse(GetViewProjectionMatrix());
    const Vec4 nearPoint = inverseViewProjection * Vec4(ndc.x, ndc.y, 0.0f, 1.0f);
    const Vec4 farPoint = inverseViewProjection * Vec4(ndc.x, ndc.y, 1.0f, 1.0f);
    return glm::normalize(Vec3(farPoint) / farPoint.w - Vec3(nearPoint) / nearPoint.w);
}

void CameraNode::RecalculateProjection() {
    if (m_ProjectionType == ProjectionType::Perspective) {
        m_Projection = glm::perspectiveLH(glm::radians(m_PerspectiveFOV), m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
    } else{
        const float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
        const float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
        const float orthoBottom = -m_OrthographicSize * 0.5f;
        const float orthoTop = m_OrthographicSize * 0.5f;
        m_Projection = glm::orthoLH(orthoLeft, orthoRight,
            orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
    }
    // GLM projections target GL clip space (+Y up in NDC); Vulkan's viewport maps +Y down.
    // Flipping the Y row keeps world +Y up on screen. Scene pipelines rasterize clockwise
    // front faces to match the mirrored winding.
    m_Projection[1][1] *= -1.0f;
    TickTransform(true);
}