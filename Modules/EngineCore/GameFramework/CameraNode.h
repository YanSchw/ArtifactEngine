#pragma once
#include "Node3D.h"
#include "Object/Enum.h"
#include "CameraNode.gen.h"

ARTIFACT_ENUM();
enum class CameraProjectionType : uint8_t { Perspective = 0, Orthographic = 1 };

class CameraNode : public Node3D {
public:
    ARTIFACT_CLASS();
    using ProjectionType = CameraProjectionType;

public:
    CameraNode();
    void BeginPlay() override;
    virtual void TickTransform(bool inWorldSpace) override;
    FUNCTION()
    void SetPerspective(float verticalFOV, float nearClip, float farClip);
    FUNCTION()
    void SetOrthographic(float size, float nearClip, float farClip);
    void SetViewportSize(uint32_t width, uint32_t height);
    FUNCTION()
    void SetAspectRatio(float aspect) { m_AspectRatio = aspect; RecalculateProjection(); }
    FUNCTION(Pure)
    float GetAspectRatio() const { return m_AspectRatio; }
    FUNCTION(Pure)
    float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
    FUNCTION()
    void SetPerspectiveVerticalFOV(float verticalFov) { m_PerspectiveFOV = verticalFov; RecalculateProjection(); }
    FUNCTION(Pure)
    float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
    FUNCTION()
    void SetPerspectiveNearClip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
    FUNCTION(Pure)
    float GetPerspectiveFarClip() const { return m_PerspectiveFar; }
    FUNCTION()
    void SetPerspectiveFarClip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }
    FUNCTION(Pure)
    float GetOrthographicSize() const { return m_OrthographicSize; }
    FUNCTION()
    void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
    FUNCTION(Pure)
    float GetOrthographicNearClip() const { return m_OrthographicNear; }
    FUNCTION()
    void SetOrthographicNearClip(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
    FUNCTION(Pure)
    float GetOrthographicFarClip() const { return m_OrthographicFar; }
    FUNCTION()
    void SetOrthographicFarClip(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }
    FUNCTION(Pure)
    CameraProjectionType GetProjectionType() const { return m_ProjectionType; }
    FUNCTION()
    void SetProjectionType(CameraProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }
    FUNCTION(Pure)
    Vec3 ScreenPosToWorldDirection(const Vec2& InScreenPos, float InWindowWidth, float InWindowHeight) const;

    public:
    void RecalculateProjection();
    const Mat4& GetProjectionMatrix() const { return m_Projection; }
    const Mat4& GetViewProjectionMatrix() const { return m_ViewProjection; }
    FUNCTION()
    void SetClearColor(const Color& color) { m_ClearColor = color; }
    FUNCTION(Pure)
    Color GetClearColor() const { return m_ClearColor; }
    FUNCTION(Pure)
    bool IsPostProcessingEnabled() const { return m_EnablePostProcessing; }
    FUNCTION()
    void SetPostProcessingEnabled(bool InEnabled) { m_EnablePostProcessing = InEnabled; }

protected:
    Mat4 m_Projection = Mat4(1.0f);
    Mat4 m_ViewProjection = Mat4(1.0f);

    PROPERTY()
    bool m_EnablePostProcessing = true;

private:
    PROPERTY()
    CameraProjectionType m_ProjectionType = CameraProjectionType::Perspective;

    /** Vertical field of view in degrees. */
    PROPERTY(Range = (1, 179))
    float m_PerspectiveFOV = 65.0f;

    PROPERTY()
    float m_PerspectiveNear = 0.01f;

    PROPERTY()
    float m_PerspectiveFar = 1000.0f;

    /** Height of the view in world units. */
    PROPERTY()
    float m_OrthographicSize = 10.0f;

    PROPERTY()
    float m_OrthographicNear = -1.0f;

    PROPERTY()
    float m_OrthographicFar = 1.0f;

    float m_AspectRatio = 1.0f;

    PROPERTY()
    Color m_ClearColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
};
