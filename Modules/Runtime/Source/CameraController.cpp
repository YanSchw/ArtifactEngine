#include "CameraController.h"

#include "Window.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/InputActionMap.h"
#include "InputSystem/InputAction.h"
#include "InputSystem/InputBinding.h"
#include "InputSystem/GamepadDevice.h"

#define LOOK_SENSITIVITY 7.0f

void CameraController::BeginPlay() {
    Super::BeginPlay();

    Window::GetInstance()->SetCursorLocked(true);

    // Drive the camera every frame.
    SetUpdateFlag(UpdateFlag::WorldUpdate);

    // Seed yaw/pitch from whatever rotation we spawned with.
    Vec3 euler = GetEulerRotation();
    m_Pitch = euler.x;
    m_Yaw = euler.y;

    InputActionMap* map = new InputActionMap();
    map->Name = "CameraController";

    // Move: WASD as a normalized Vec2, or the gamepad left stick.
    m_MoveAction = new InputAction();
    m_MoveAction->Name = "Move";
    m_MoveAction->Type = InputValueType::Vec2;
    {
        Vector2Composite* wasd = new Vector2Composite();
        wasd->Up = "Keyboard/W";
        wasd->Down = "Keyboard/S";
        wasd->Left = "Keyboard/A";
        wasd->Right = "Keyboard/D";
        m_MoveAction->Bindings.Add(wasd);

        PathBinding* stick = new PathBinding();
        stick->Path = "Gamepad/LeftStick";
        m_MoveAction->Bindings.Add(stick);
    }
    map->Actions.Add(m_MoveAction);

    // Look: mouse delta (pixels/frame) or the gamepad right stick.
    m_LookAction = new InputAction();
    m_LookAction->Name = "Look";
    m_LookAction->Type = InputValueType::Vec2;
    {
        PathBinding* mouse = new PathBinding();
        mouse->Path = "Mouse/Delta";
        m_LookAction->Bindings.Add(mouse);

        PathBinding* stick = new PathBinding();
        stick->Path = "Gamepad/RightStick";
        m_LookAction->Bindings.Add(stick);
    }
    map->Actions.Add(m_LookAction);

    // Fly: vertical movement, Space up / Left-Ctrl down.
    m_FlyAction = new InputAction();
    m_FlyAction->Name = "Fly";
    m_FlyAction->Type = InputValueType::Float;
    {
        Axis1DComposite* fly = new Axis1DComposite();
        fly->Positive = "Keyboard/Q";
        fly->Negative = "Keyboard/E";
        m_FlyAction->Bindings.Add(fly);
    }
    map->Actions.Add(m_FlyAction);

    // InputSystem keeps the map alive and evaluates it each Tick.
    InputSystem::Get().AddActionMap(map);
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
