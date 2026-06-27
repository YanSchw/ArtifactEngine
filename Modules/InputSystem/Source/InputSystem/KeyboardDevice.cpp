#include "KeyboardDevice.h"
#include "InputSystem.h"

KeyboardDevice::KeyboardDevice() {
    for (KeyCode key : EKeyCode::GetValues()) {
        m_Keys[key] = false;
    }

    m_KeysLastFrame = m_Keys;
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