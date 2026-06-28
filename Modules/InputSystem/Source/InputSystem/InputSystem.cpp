#include "InputSystem.h"
#include "InputDevice.h"
#include "InputActionMap.h"

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

Array<SharedObjectPtr<InputActionMap>> InputSystem::GetActionMaps() const {
    return m_ActionMaps;
}

void InputSystem::AddActionMap(InputActionMap* InMap) {
    AE_ASSERT(InMap);
    m_ActionMaps.Add(InMap);
}

void InputSystem::RemoveActionMap(InputActionMap* InMap) {
    AE_ASSERT(InMap);
    for (int32_t i = 0; i < m_ActionMaps.Size(); i++) {
        if (m_ActionMaps[i] == InMap) {
            m_ActionMaps.RemoveAt(i);
            return;
        }
    }
}

bool InputSystem::ReadPath(const String& InPath, InputValue& OutValue) {
    size_t slash = InPath.find('/');
    if (slash == String::npos) {
        return false;
    }
    String device = InPath.substr(0, slash);
    String control = InPath.substr(slash + 1);
    for (int32_t i = 0; i < m_Devices.Size(); i++) {
        const SharedObjectPtr<InputDevice>& it = m_Devices[i];
        if (it->GetDeviceName() == device) {
            if (!it->ReadControl(control, OutValue)) {
                return false;
            }
            // Stamp the source so callbacks can tell which device fired.
            OutValue.Device = it;
            OutValue.DeviceIndex = i;
            return true;
        }
    }
    return false;
}

WeakObjectPtr<InputDevice> InputSystem::GetLastActiveDevice() const {
    return m_LastActiveDevice;
}

void InputSystem::SetLastActiveDevice(const WeakObjectPtr<InputDevice>& InDevice) {
    m_LastActiveDevice = InDevice;
}

Array<String> InputSystem::GetControlPaths(InputValueType InFilter) {
    Array<String> paths;
    for (const auto& device : m_Devices) {
        const String prefix = device->GetDeviceName() + "/";
        for (const InputControl& control : device->GetControls()) {
            if (control.Type == InFilter) {
                paths.Add(prefix + control.Name);
            }
        }
    }
    return paths;
}

void InputSystem::Tick(float InDeltatime) {
    for (int32_t i = m_Devices.Last(); i >= 0; i--) {
        m_Devices[i]->Tick();
    }

    // Devices are now up to date for this frame; evaluate actions on top of them.
    for (const auto& actionMap : m_ActionMaps) {
        actionMap->Evaluate();
    }
}