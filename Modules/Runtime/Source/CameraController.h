#pragma once
#include "GameFramework/CameraNode.h"
#include "CameraController.gen.h"

class InputAction;

// A free-fly first-person camera driven by the InputSystem:
//   Move : WASD / left stick   Look : mouse / right stick   Fly : Q-E
class CameraController : public CameraNode {
public:
    ARTIFACT_CLASS();

    virtual void BeginPlay() override;
    virtual void WorldUpdate(float InDeltatime) override;

private:
    InputAction* m_MoveAction = nullptr;
    InputAction* m_LookAction = nullptr;
    InputAction* m_FlyAction = nullptr;

    float m_Yaw = 0.0f;    // degrees, around world up
    float m_Pitch = 0.0f;  // degrees, clamped to avoid gimbal flip

    float m_MoveSpeed = 5.0f;         // units / second
    float m_LookSensitivity = 0.1f;   // degrees per unit of look input
};
