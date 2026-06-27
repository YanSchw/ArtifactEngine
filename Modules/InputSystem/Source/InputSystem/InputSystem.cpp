#include "InputSystem.h"
#include "InputDevice.h"

InputSystem& InputSystem::Get() {
    static InputSystem s_InputSystem;
    return s_InputSystem;
}

Array<SharedObjectPtr<InputDevice>> InputSystem::GetDevices() const {
    return m_Devices;
}

void InputSystem::AddDevice(InputDevice* InDevice) {
    AE_ASSERT(InDevice);
    m_Devices.Add(InDevice);
    AE_INFO("Registered InputDevice: {}", InDevice->GetClass().Name);
}

void InputSystem::RemoveDevice(InputDevice* InDevice) {
    AE_ASSERT(InDevice);
    for (uint32_t i = 0; i < m_Devices.Size(); i++) {
        if (m_Devices[i] == InDevice) {
            AE_INFO("Unregistered InputDevice: {}", InDevice->GetClass().Name);
            m_Devices.RemoveAt(i);
            return;
        }
    }
}

void InputSystem::Tick(float InDeltatime) {
    for (int32_t i = m_Devices.Last(); i >= 0; i--) {
        m_Devices[i]->Tick();
    }
}