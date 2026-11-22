#include "MacOSGamepad.h"
#include "MacOSGamepadBridge.h"
#include "InputSystem/InputSystem.h"

void MacOSGamepadSubsystem::Initialize() {
    InputSystem::Get().AddDevice(new GameControllerGamepadDevice());
}

void GameControllerGamepadDevice::Tick() {
    Super::Tick();

    MacOSGamepadState state;
    if (!ReadMacOSGamepad(state)) {
        ResetState();
        return;
    }
    m_Connected = true;

    static_assert((int)GamepadCode::DPadLeft == 14, "MacOSGamepadState::Buttons is indexed by GamepadCode");
    for (auto& [button, pressed] : m_Buttons) {
        pressed = state.Buttons[(int)button];
    }
    m_LeftStick = ApplyStickDeadzone({ state.LeftStick[0], state.LeftStick[1] });
    m_RightStick = ApplyStickDeadzone({ state.RightStick[0], state.RightStick[1] });
    m_LeftTrigger = state.LeftTrigger;
    m_RightTrigger = state.RightTrigger;
}

void GameControllerGamepadDevice::ApplyMotorSpeeds(float InLowFrequency, float InHighFrequency) {
    SetMacOSGamepadMotorSpeeds(InLowFrequency, InHighFrequency);
}
