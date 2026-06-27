#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "InputSystem.gen.h"

class InputDevice;

class InputSystem : public Object {
public:
    ARTIFACT_CLASS();

    static InputSystem& Get();

    Array<SharedObjectPtr<InputDevice>> GetDevices() const;
    void AddDevice(InputDevice* InDevice);
    void RemoveDevice(InputDevice* InDevice);

    void Tick(float InDeltatime);

private:
    Array<SharedObjectPtr<InputDevice>> m_Devices;
};