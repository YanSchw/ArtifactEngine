#include "VirtualGamepadDevice.h"

void VirtualGamepadState::SetButton(GamepadCode InCode, bool InPressed) {
    const uint32_t bit = 1u << (uint32_t)InCode;
    Buttons = InPressed ? (Buttons | bit) : (Buttons & ~bit);
}

bool VirtualGamepadState::IsButtonPressed(GamepadCode InCode) const {
    return (Buttons & (1u << (uint32_t)InCode)) != 0;
}

void VirtualGamepadDevice::Submit(const VirtualGamepadState& InState) {
    m_Pending = InState;
    m_HasPending = true;
}

void VirtualGamepadDevice::Tick() {
    Super::Tick();

    const VirtualGamepadState state = m_HasPending ? m_Pending : VirtualGamepadState();
    m_HasPending = false;
    m_Connected = true;

    for (auto& [button, pressed] : m_Buttons) {
        pressed = state.IsButtonPressed(button);
    }
    m_LeftStick = ApplyStickDeadzone(state.LeftStick);
    m_RightStick = ApplyStickDeadzone(state.RightStick);
    m_LeftTrigger = state.LeftTrigger;
    m_RightTrigger = state.RightTrigger;
}
