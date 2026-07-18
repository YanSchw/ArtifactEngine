#include "EditorCamera.h"
#include "Window.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/MouseDevice.h"
#include "InputSystem/MouseCodes.h"
#include <cmath>

static constexpr float s_MinFlySpeed = 0.25f;
static constexpr float s_MaxFlySpeed = 100.0f;
static constexpr float s_SpeedStepFactor = 1.25f;
static constexpr float s_DollyStepFactor = 0.25f;
static constexpr float s_PanUnitsPerPixel = 0.002f;
static constexpr float s_MaxLookPixelsPerFrame = 60.0f;

EditorCamera::EditorCamera() {
    SetName("EditorCamera");
    SetPosition(Vec3(0.0f, 2.0f, -6.0f));
    m_Pitch = -15.0f;
    ApplyYawPitch();
}

void EditorCamera::UpdateNavigation(float InDeltaTime, Window& InWindow, bool InViewportHovered) {
    const bool rightDown = InWindow.IsMouseButtonDown((int32_t)MouseCode::Right);
    const bool middleDown = InWindow.IsMouseButtonDown((int32_t)MouseCode::Middle);

    if (m_Mode == NavMode::None && InViewportHovered) {
        if (rightDown && !m_WasRightDown) {
            BeginNavigation(NavMode::Fly, InWindow);
        } else if (middleDown && !m_WasMiddleDown) {
            BeginNavigation(NavMode::Pan, InWindow);
        }
    }
    m_WasRightDown = rightDown;
    m_WasMiddleDown = middleDown;

    if (m_Mode == NavMode::None) {
        return;
    }
    if ((m_Mode == NavMode::Fly && !rightDown) || (m_Mode == NavMode::Pan && !middleDown)) {
        CancelNavigation();
        return;
    }

    const Vec2 cursor = InWindow.GetCursorPosition();
    Vec2 cursorDelta = cursor - m_LastCursor;
    m_LastCursor = cursor;

    // Reject implausibly large single-frame cursor jumps.
    if (std::abs(cursorDelta.x) > s_MaxLookPixelsPerFrame ||
        std::abs(cursorDelta.y) > s_MaxLookPixelsPerFrame) {
        cursorDelta = Vec2(0.0f);
    }

    if (m_Mode == NavMode::Fly) {
        UpdateFly(InDeltaTime, cursorDelta);
    } else {
        UpdatePan(cursorDelta);
    }
}

void EditorCamera::BeginNavigation(NavMode InMode, Window& InWindow) {
    m_Mode = InMode;
    m_NavWindow = &InWindow;
    InWindow.SetCursorLocked(true);
    m_LastCursor = InWindow.GetCursorPosition();
}

void EditorCamera::CancelNavigation() {
    if (Window* window = m_NavWindow.Get()) {
        window->SetCursorLocked(false);
    }
    m_NavWindow = nullptr;
    m_Mode = NavMode::None;
}

void EditorCamera::UpdateFly(float InDeltaTime, const Vec2& InCursorDelta) {
    m_Yaw += InCursorDelta.x * m_LookSensitivity;
    m_Pitch -= InCursorDelta.y * m_LookSensitivity;
    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
    ApplyYawPitch();

    if (MouseDevice* mouse = MouseDevice::Instance()) {
        const float steps = mouse->GetScrollDelta().y;
        if (steps != 0.0f) {
            m_FlySpeed = glm::clamp(m_FlySpeed * std::pow(s_SpeedStepFactor, steps), s_MinFlySpeed, s_MaxFlySpeed);
        }
    }

    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (!keyboard) {
        return;
    }
    const auto axis = [keyboard](KeyCode InPositive, KeyCode InNegative) {
        return (keyboard->IsPressed(InPositive) ? 1.0f : 0.0f) - (keyboard->IsPressed(InNegative) ? 1.0f : 0.0f);
    };
    Vec3 velocity = GetForwardVector() * axis(KeyCode::W, KeyCode::S)
                  + GetRightVector() * axis(KeyCode::D, KeyCode::A)
                  + VecUtils::Up * axis(KeyCode::Q, KeyCode::E);
    if (glm::length(velocity) > 1.0f) {
        velocity = glm::normalize(velocity);
    }
    AddWorldOffset(velocity * m_FlySpeed * InDeltaTime);
}

void EditorCamera::UpdatePan(const Vec2& InCursorDelta) {
    const float unitsPerPixel = m_FlySpeed * s_PanUnitsPerPixel;
    AddWorldOffset((GetRightVector() * InCursorDelta.x + GetUpVector() * InCursorDelta.y) * unitsPerPixel);
}

void EditorCamera::Dolly(float InSteps) {
    AddWorldOffset(GetForwardVector() * InSteps * m_FlySpeed * s_DollyStepFactor);
}

void EditorCamera::ApplyYawPitch() {
    SetRotation(glm::angleAxis(glm::radians(m_Yaw), VecUtils::Up) *
                glm::angleAxis(glm::radians(m_Pitch), Vec3(1.0f, 0.0f, 0.0f)));
}
