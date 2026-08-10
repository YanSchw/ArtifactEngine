#include "BrowserGamepadDevice.h"

#include <emscripten/html5.h>

// Where each GamepadCode sits in the W3C standard mapping, in GamepadCode order.
static constexpr int32_t s_StandardButtons[] = { 0, 1, 2, 3, 4, 5, 8, 9, 16, 10, 11, 12, 15, 13, 14 };
static constexpr int32_t s_LeftTriggerButton = 6;
static constexpr int32_t s_RightTriggerButton = 7;

static float ReadAxis(const EmscriptenGamepadEvent& InState, int32_t InAxis) {
    return InAxis < InState.numAxes ? (float)InState.axis[InAxis] : 0.0f;
}

static float ReadAnalogButton(const EmscriptenGamepadEvent& InState, int32_t InButton) {
    return InButton < InState.numButtons ? (float)InState.analogButton[InButton] : 0.0f;
}

/** The first connected pad, preferring one the browser could fit to the standard mapping. */
static bool FindGamepad(EmscriptenGamepadEvent& OutState) {
    const int32_t count = emscripten_get_num_gamepads();
    bool found = false;

    for (int32_t index = 0; index < count; index++) {
        EmscriptenGamepadEvent state;
        if (emscripten_get_gamepad_status(index, &state) != EMSCRIPTEN_RESULT_SUCCESS || !state.connected) {
            continue;
        }
        if (String(state.mapping) == "standard") {
            OutState = state;
            return true;
        }
        if (!found) {
            OutState = state;
            found = true;
        }
    }
    return found;
}

void BrowserGamepadDevice::Tick() {
    Super::Tick();

    EmscriptenGamepadEvent state;
    if (emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS || !FindGamepad(state)) {
        ResetState();
        return;
    }

    m_Connected = true;

    for (auto& [button, pressed] : m_Buttons) {
        const int32_t code = (int32_t)button;
        const int32_t index = code < (int32_t)std::size(s_StandardButtons) ? s_StandardButtons[code] : -1;
        pressed = index >= 0 && index < state.numButtons && state.digitalButton[index];
    }

    // The browser reports stick Y as down-positive; flip so up is +Y (matches WASD).
    m_LeftStick = ApplyStickDeadzone(Vec2(ReadAxis(state, 0), -ReadAxis(state, 1)));
    m_RightStick = ApplyStickDeadzone(Vec2(ReadAxis(state, 2), -ReadAxis(state, 3)));

    m_LeftTrigger = ReadAnalogButton(state, s_LeftTriggerButton);
    m_RightTrigger = ReadAnalogButton(state, s_RightTriggerButton);
}
