#include "KeyboardDevice.h"
#include "InputSystem.h"

KeyboardDevice::KeyboardDevice() {
    for (KeyCode key : EKeyCode::GetValues()) {
        m_Keys[key] = false;
        m_NameToKey[EKeyCode::ConvertEnumToString(key)] = key;
    }

    m_KeysLastFrame = m_Keys;
}

String KeyboardDevice::GetDeviceName() const {
    return "Keyboard";
}

Array<InputControl> KeyboardDevice::GetControls() const {
    Array<InputControl> controls;
    for (KeyCode key : EKeyCode::GetValues()) {
        controls.Add({EKeyCode::ConvertEnumToString(key), InputValueType::Bool});
    }
    return controls;
}

bool KeyboardDevice::ReadControl(const String& InControl, InputValue& OutValue) {
    if (!m_NameToKey.ContainsKey(InControl)) {
        return false;
    }
    KeyCode key = m_NameToKey.At(InControl);
    OutValue = {{IsPressed(key) ? 1.0f : 0.0f, 0.0f}};
    return true;
}

bool KeyboardDevice::IsPressed(KeyCode InCode) {
    return m_Keys.At(InCode);
}

bool KeyboardDevice::IsDown(KeyCode InCode) {
    return m_Keys.At(InCode) && !m_KeysLastFrame.At(InCode);
}

bool KeyboardDevice::IsUp(KeyCode InCode) {
    return !m_Keys.At(InCode) && m_KeysLastFrame.At(InCode);
}

KeyboardDevice* KeyboardDevice::Instance() {
    for (const auto& It : InputSystem::Get().GetDevices()) {
        if (It->IsA<KeyboardDevice>()) {
            return It->As<KeyboardDevice>();
        }
    }
    return nullptr;
}

void KeyboardDevice::Tick() {
    m_KeysLastFrame = m_Keys;
}