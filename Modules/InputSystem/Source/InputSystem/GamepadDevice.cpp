#include "GamepadDevice.h"
#include "InputSystem.h"

GamepadDevice::GamepadDevice() {
    for (GamepadCode button : EGamepadCode::GetValues()) {
        m_Buttons[button] = false;
        m_NameToButton[EGamepadCode::ConvertEnumToString(button)] = button;
    }

    m_ButtonsLastFrame = m_Buttons;
}

bool GamepadDevice::IsPressed(GamepadCode InCode) {
    return m_Buttons.At(InCode);
}

bool GamepadDevice::IsDown(GamepadCode InCode) {
    return m_Buttons.At(InCode) && !m_ButtonsLastFrame.At(InCode);
}

bool GamepadDevice::IsUp(GamepadCode InCode) {
    return !m_Buttons.At(InCode) && m_ButtonsLastFrame.At(InCode);
}

Vec2 GamepadDevice::GetLeftStick() const {
    return m_LeftStick;
}

Vec2 GamepadDevice::GetRightStick() const {
    return m_RightStick;
}

float GamepadDevice::GetLeftTrigger() const {
    return m_LeftTrigger;
}

float GamepadDevice::GetRightTrigger() const {
    return m_RightTrigger;
}

Vec2 GamepadDevice::GetDPad() {
    float x = (m_Buttons.At(GamepadCode::DPadRight) ? 1.0f : 0.0f) -
              (m_Buttons.At(GamepadCode::DPadLeft) ? 1.0f : 0.0f);
    float y = (m_Buttons.At(GamepadCode::DPadUp) ? 1.0f : 0.0f) -
              (m_Buttons.At(GamepadCode::DPadDown) ? 1.0f : 0.0f);
    return {x, y};
}

bool GamepadDevice::IsConnected() const {
    return m_Connected;
}

String GamepadDevice::GetDeviceName() const {
    return "Gamepad";
}

Array<InputControl> GamepadDevice::GetControls() const {
    Array<InputControl> controls;
    for (GamepadCode button : EGamepadCode::GetValues()) {
        controls.Add({EGamepadCode::ConvertEnumToString(button), InputValueType::Bool});
    }
    controls.Add({"LeftStick", InputValueType::Vec2});
    controls.Add({"RightStick", InputValueType::Vec2});
    controls.Add({"DPad", InputValueType::Vec2});
    controls.Add({"LeftTrigger", InputValueType::Float});
    controls.Add({"RightTrigger", InputValueType::Float});
    return controls;
}

bool GamepadDevice::ReadControl(const String& InControl, InputValue& OutValue) {
    if (InControl == "LeftStick") {
        OutValue = {m_LeftStick};
        return true;
    }
    if (InControl == "RightStick") {
        OutValue = {m_RightStick};
        return true;
    }
    if (InControl == "DPad") {
        OutValue = {GetDPad()};
        return true;
    }
    if (InControl == "LeftTrigger") {
        OutValue = {{m_LeftTrigger, 0.0f}};
        return true;
    }
    if (InControl == "RightTrigger") {
        OutValue = {{m_RightTrigger, 0.0f}};
        return true;
    }
    if (m_NameToButton.ContainsKey(InControl)) {
        GamepadCode button = m_NameToButton.At(InControl);
        OutValue = {{IsPressed(button) ? 1.0f : 0.0f, 0.0f}};
        return true;
    }
    return false;
}

GamepadDevice* GamepadDevice::Instance() {
    for (const auto& It : InputSystem::Get().GetDevices()) {
        if (It->IsA<GamepadDevice>()) {
            return It->As<GamepadDevice>();
        }
    }
    return nullptr;
}

void GamepadDevice::Tick() {
    m_ButtonsLastFrame = m_Buttons;
}
