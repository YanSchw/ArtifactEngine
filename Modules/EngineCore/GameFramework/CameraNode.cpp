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

// https://stackoverflow.com/questions/29997209/opengl-c-mouse-ray-picking-glmunproject
Vec3 CameraNode::ScreenPosToWorldDirection(const Vec2& pos, float windowWidth, float windowHeight) const {
    // these positions must be in range [-1, 1] (!!!), not [0, width] and [0, height]
    const float mouseX = pos.x / (windowWidth * 0.5f) - 1.0f;
    const float mouseY = pos.y / (windowHeight * 0.5f) - 1.0f;
    Mat4 proj = glm::perspectiveLH(glm::radians(m_PerspectiveFOV), m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
    proj[1][1] *= -1.0f;
    Mat4 view = glm::lookAt(Vec3(0.0f), GetForwardVector(), GetUpVector());
    Mat4 invVP = glm::inverse(proj * view);
    Vec4 screenPos = Vec4(mouseX, mouseY, 1.0f, 1.0f);
    Vec4 worldPos = invVP * screenPos;
    Vec3 dir = glm::normalize(Vec3(worldPos));
    return dir * -1.0f;
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