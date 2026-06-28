#include "GLFWGamepadDevice.h"

#include <GLFW/glfw3.h>

// Sticks never rest at exactly zero; ignore anything inside this radius and
// rescale the rest so motion still starts smoothly at the deadzone edge.
static constexpr float k_StickDeadzone = 0.15f;

static Vec2 ApplyRadialDeadzone(Vec2 InStick) {
    float length = glm::length(InStick);
    if (length < k_StickDeadzone) {
        return {0.0f, 0.0f};
    }
    float rescaled = (length - k_StickDeadzone) / (1.0f - k_StickDeadzone);
    return InStick * (rescaled / length);
}

void GLFWGamepadDevice::Tick() {
    Super::Tick();

    GLFWgamepadstate state;
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state) != GLFW_TRUE) {
        // No pad on this slot: report disconnected and zero everything.
        m_Connected = false;
        for (auto& [button, pressed] : m_Buttons) {
            pressed = false;
        }
        m_LeftStick = {0.0f, 0.0f};
        m_RightStick = {0.0f, 0.0f};
        m_LeftTrigger = 0.0f;
        m_RightTrigger = 0.0f;
        return;
    }

    m_Connected = true;

    for (auto& [button, pressed] : m_Buttons) {
        pressed = (state.buttons[static_cast<int32_t>(button)] == GLFW_PRESS);
    }

    // GLFW reports stick Y as down-positive; flip so up is +Y (matches WASD).
    m_LeftStick = ApplyRadialDeadzone({state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], -state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]});
    m_RightStick = ApplyRadialDeadzone({state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], -state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]});

    // Triggers arrive in [-1, 1] (released..pressed); remap to [0, 1].
    m_LeftTrigger = (state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0f) * 0.5f;
    m_RightTrigger = (state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0f) * 0.5f;
}
