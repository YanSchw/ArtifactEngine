#include "GLFWGamepadDevice.h"

#include <GLFW/glfw3.h>

void GLFWGamepadDevice::Tick() {
    Super::Tick();

    GLFWgamepadstate state;
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state) != GLFW_TRUE) {
        ResetState();
        return;
    }

    m_Connected = true;

    for (auto& [button, pressed] : m_Buttons) {
        pressed = (state.buttons[static_cast<int32_t>(button)] == GLFW_PRESS);
    }

    // GLFW reports stick Y as down-positive; flip so up is +Y (matches WASD).
    m_LeftStick = ApplyStickDeadzone({state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], -state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]});
    m_RightStick = ApplyStickDeadzone({state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], -state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]});

    // Triggers arrive in [-1, 1] (released..pressed); remap to [0, 1].
    m_LeftTrigger = (state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0f) * 0.5f;
    m_RightTrigger = (state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0f) * 0.5f;
}
