#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "InputValue.h"
#include "InputSystem.gen.h"

class InputDevice;
class InputActionMap;

class InputSystem : public Object {
public:
    ARTIFACT_CLASS();

    static InputSystem& Get();

    Array<SharedObjectPtr<InputDevice>> GetDevices() const;
    void AddDevice(InputDevice* InDevice);
    void RemoveDevice(InputDevice* InDevice);

    Array<SharedObjectPtr<InputActionMap>> GetActionMaps() const;
    void AddActionMap(InputActionMap* InMap);
    void RemoveActionMap(InputActionMap* InMap);

    // Resolve a control path like "Keyboard/W" against the connected devices.
    // Returns false (and leaves OutValue at its default) if nothing matches.
    bool ReadPath(const String& InPath, InputValue& OutValue);

    // Every available control path of the given value type, e.g. for an editor
    // binding dropdown: "Keyboard/W", "Mouse/Delta", ...
    Array<String> GetControlPaths(InputValueType InFilter);

    void Tick(float InDeltatime);

private:
    Array<SharedObjectPtr<InputDevice>> m_Devices;
    Array<SharedObjectPtr<InputActionMap>> m_ActionMaps;
};