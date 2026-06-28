#pragma once
#include "InputSystem/GamepadDevice.h"
#include "GLFWGamepadDevice.gen.h"

// Single-player GLFW gamepad backend: always polls the first joystick slot
// (GLFW_JOYSTICK_1). Multi-pad support comes later.
class GLFWGamepadDevice : public GamepadDevice {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Tick() override;
};
