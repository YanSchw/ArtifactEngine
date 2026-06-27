#pragma once
#include "Node3D.h"
#include "CameraNode.gen.h"

class CameraNode : public Node3D {
public:
    ARTIFACT_CLASS();
    enum class ProjectionType { Perspective = 0, Orthographic = 1 };

public:
    CameraNode();
    void BeginPlay() override;
    virtual void TickTransform(bool inWorldSpace) override;
    void SetPerspective(float verticalFOV, float nearClip, float farClip);
    void SetOrthographic(float size, float nearClip, float farClip);
    void SetViewportSize(uint32_t width, uint32_t height);
    void SetAspectRatio(float aspect) { m_AspectRatio = aspect; RecalculateProjection(); }
    float GetAspectRatio() const { return m_AspectRatio; }
    float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
    void SetPerspectiveVerticalFOV(float verticalFov) { m_PerspectiveFOV = verticalFov; RecalculateProjection(); }
    float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
    void SetPerspectiveNearClip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
    float GetPerspectiveFarClip() const { return m_PerspectiveFar; }
    void SetPerspectiveFarClip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }
    float GetOrthographicSize() const { return m_OrthographicSize; }
    void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
    float GetOrthographicNearClip() const { return m_OrthographicNear; }
    void SetOrthographicNearClip(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
    float GetOrthographicFarClip() const { return m_OrthographicFar; }
    void SetOrthographicFarClip(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }
    ProjectionType GetProjectionType() const { return m_ProjectionType; }
    void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }
    Vec3 ScreenPosToWorldDirection(const Vec2& pos, float windowWidth, float windowHeight) const;

    public:
    void RecalculateProjection();
    const Mat4& GetProjectionMatrix() const { return m_Projection; }
    const Mat4& GetViewProjectionMatrix() const { return m_ViewProjection; }
    void SetClearColor(Vec4 color) { m_ClearColor = color; }
    Vec4 GetClearColor() const { return m_ClearColor; }

protected:
    Mat4 m_Projection = Mat4(1.0f);
    Mat4 m_ViewProjection = Mat4(1.0f);

private:
    ProjectionType m_ProjectionType = ProjectionType::Perspective;
    float m_PerspectiveFOV = 65.0f;
    float m_PerspectiveNear = 0.01f, m_PerspectiveFar = 1000.0f;
    float m_OrthographicSize = 10.0f;
    float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;
    float m_AspectRatio = 1.0f;
    Vec4 m_ClearColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
};