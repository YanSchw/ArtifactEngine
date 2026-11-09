#pragma once
#include "GameFramework/CameraNode.h"
#include "Object/Pointer.h"
#include "EditorCamera.gen.h"

class Window;

class EditorCamera : public CameraNode {
public:
    ARTIFACT_CLASS();

    inline static constexpr float MinFlySpeed = 0.25f;
    inline static constexpr float MaxFlySpeed = 100.0f;

    EditorCamera();

    void UpdateNavigation(float InDeltaTime, Window& InWindow, bool InViewportHovered);
    void CancelNavigation();

    void Dolly(float InSteps);
    void SetOrthographicView(bool InOrthographic);
    bool IsOrthographicView() const { return GetProjectionType() == ProjectionType::Orthographic; }

    bool IsNavigating() const { return m_Mode != NavMode::None; }

private:
    enum class NavMode : uint8_t { None = 0, Fly, Pan };

    void BeginNavigation(NavMode InMode, Window& InWindow);
    void UpdateFly(float InDeltaTime, const Vec2& InCursorDelta);
    void UpdatePan(const Vec2& InCursorDelta);
    void ApplyYawPitch();

    NavMode m_Mode = NavMode::None;
    WeakObjectPtr<Window> m_NavWindow;
    Vec2 m_LastCursor = Vec2(0.0f);
    bool m_WasRightDown = false;
    bool m_WasMiddleDown = false;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_FlySpeed = 5.0f;
};
