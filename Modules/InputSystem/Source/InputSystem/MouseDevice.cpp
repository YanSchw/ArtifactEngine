#include "MouseDevice.h"
#include "InputSystem.h"

MouseDevice::MouseDevice() {
    for (MouseCode button : EMouseCode::GetValues()) {
        m_Buttons[button] = false;
        m_NameToButton[EMouseCode::ConvertEnumToString(button)] = button;
    }

    m_ButtonsLastFrame = m_Buttons;
}

String MouseDevice::GetDeviceName() const {
    return "Mouse";
}

Array<InputControl> MouseDevice::GetControls() const {
    Array<InputControl> controls;
    for (MouseCode button : EMouseCode::GetValues()) {
        controls.Add({EMouseCode::ConvertEnumToString(button), InputValueType::Bool});
    }
    controls.Add({"Delta", InputValueType::Vec2});
    controls.Add({"Position", InputValueType::Vec2});
    controls.Add({"Scroll", InputValueType::Vec2});
    return controls;
}

bool MouseDevice::ReadControl(const String& InControl, InputValue& OutValue) {
    if (InControl == "Delta") {
        OutValue = {GetPositionDelta()};
        return true;
    }
    if (InControl == "Position") {
        OutValue = {GetPosition()};
        return true;
    }
    if (InControl == "Scroll") {
        OutValue = {GetScrollDelta()};
        return true;
    }
    if (m_NameToButton.ContainsKey(InControl)) {
        MouseCode button = m_NameToButton.At(InControl);
        OutValue = {{IsPressed(button) ? 1.0f : 0.0f, 0.0f}};
        return true;
    }
    return false;
}

bool MouseDevice::IsPressed(MouseCode InCode) {
    return m_Buttons.At(InCode);
}

bool MouseDevice::IsDown(MouseCode InCode) {
    return m_Buttons.At(InCode) && !m_ButtonsLastFrame.At(InCode);
}

bool MouseDevice::IsUp(MouseCode InCode) {
    return !m_Buttons.At(InCode) && m_ButtonsLastFrame.At(InCode);
}

Vec2 MouseDevice::GetPosition() const {
    return m_Position;
}

float MouseDevice::GetX() const {
    return m_Position.x;
}

float MouseDevice::GetY() const {
    return m_Position.y;
}

Vec2 MouseDevice::GetPositionDelta() const {
    return m_Position - m_PositionLastFrame;
}

float MouseDevice::GetDeltaX() const {
    return m_Position.x - m_PositionLastFrame.x;
}

float MouseDevice::GetDeltaY() const {
    return m_Position.y - m_PositionLastFrame.y;
}

Vec2 MouseDevice::GetScrollDelta() const {
    return m_ScrollDelta;
}

float MouseDevice::GetScrollX() const {
    return m_ScrollDelta.x;
}

float MouseDevice::GetScrollY() const {
    return m_ScrollDelta.y;
}

MouseDevice* MouseDevice::Instance() {
    for (const auto& It : InputSystem::Get().GetDevices()) {
        if (It->IsA<MouseDevice>()) {
            return It->As<MouseDevice>();
        }
    }
    return nullptr;
}

void MouseDevice::Tick() {
    m_ButtonsLastFrame = m_Buttons;
    m_PositionLastFrame = m_Position;
    // Reset the per-frame scroll accumulation; a backend refills it after Super::Tick().
    m_ScrollDelta = {0.0f, 0.0f};
}
