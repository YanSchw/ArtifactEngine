#include "CameraController.h"

#include "Window.h"
#include "Assets/AssetManager.h"
#include "Common/UUID.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/InputActionMapping.h"
#include "InputSystem/InputAction.h"
#include "InputSystem/GamepadDevice.h"

#define LOOK_SENSITIVITY 7.0f

// Content/DefaultInputMappings.asset
static const UUID s_InputMappingId = UUID::FromString("f47ac10b-58cc-4372-a567-0e02b2c3d479");

void CameraController::BeginPlay() {
    Super::BeginPlay();

    Window::GetInstance()->SetCursorLocked(true);

    // Drive the camera every frame.
    SetUpdateFlag(UpdateFlag::WorldUpdate);

    // Seed yaw/pitch from whatever rotation we spawned with.
    Vec3 euler = GetEulerRotation();
    m_Pitch = euler.x;
    m_Yaw = euler.y;

    // Load the action mapping (with all its bindings) from the asset.
    InputActionMapping* mapping = AssetManager::Get().GetAsset<InputActionMapping>(s_InputMappingId);
    AE_ASSERT(mapping, "Failed to load DefaultInputMappings.asset");

    // InputSystem keeps the mapping alive and evaluates it each Tick.
    InputSystem::Get().AddActionMapping(mapping);

    // Grab the action pointers by name for use in WorldUpdate.
    m_MoveAction = mapping->Find("Move");
    m_LookAction = mapping->Find("Look");
    m_FlyAction = mapping->Find("Fly");
    AE_ASSERT(m_MoveAction && m_LookAction && m_FlyAction, "Input mapping is missing an expected action");
}

void CameraController::WorldUpdate(float InDeltatime) {
    // Note: intentionally not calling Super::WorldUpdate — the base only logs.

    // --- Look (mouse delta is already per-frame, so no deltatime here) ---
    Vec2 look = m_LookAction->ReadVec2() * LOOK_SENSITIVITY;
    bool gamepad = InputSystem::Get().GetLastActiveDevice()->As<GamepadDevice>() != nullptr;
    m_Yaw += look.x * m_LookSensitivity;
    m_Pitch -= look.y * m_LookSensitivity * (gamepad ? -1 : 1);  // screen-y grows downward; invert for natural look
    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);

    // Build the orientation roll-free: yaw about world up, then pitch about the
    // local right axis. Reconstructing from a single euler triple couples yaw
    // and pitch into roll, which shows up as the horizon tilting diagonally.
    Quat orientation = glm::angleAxis(glm::radians(m_Yaw), VecUtils::Up) *
                       glm::angleAxis(glm::radians(m_Pitch), Vec3(1.0f, 0.0f, 0.0f));
    SetRotation(orientation);

    // --- Move relative to where we're looking ---
    Vec2 move = m_MoveAction->ReadVec2();
    float fly = m_FlyAction->ReadFloat();
    Vec3 velocity = GetForwardVector() * move.y +
                    GetRightVector() * move.x +
                    VecUtils::Up * fly;

    // Clamp to unit length so diagonals aren't faster, but keep analog magnitude.
    if (glm::length(velocity) > 1.0f) {
        velocity = glm::normalize(velocity);
    }
    AddWorldOffset(velocity * m_MoveSpeed * InDeltatime);
}
