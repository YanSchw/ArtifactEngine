#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "InputValue.h"
#include "InputSystem.gen.h"

class InputDevice;
class InputActionMapping;

class InputSystem : public Object {
public:
    ARTIFACT_CLASS();

    static InputSystem& Get();

    Array<SharedObjectPtr<InputDevice>> GetDevices() const;
    void AddDevice(InputDevice* InDevice);
    void RemoveDevice(InputDevice* InDevice);

    Array<SharedObjectPtr<InputActionMapping>> GetActionMappings() const;
    void AddActionMapping(InputActionMapping* InMapping);
    void RemoveActionMapping(InputActionMapping* InMapping);

    // Resolve a control path like "Keyboard/W" against the connected devices.
    // Returns false (and leaves OutValue at its default) if nothing matches.
    bool ReadPath(const String& InPath, InputValue& OutValue);

    // Every available control path of the given value type, e.g. for an editor
    // binding dropdown: "Keyboard/W", "Mouse/Delta", ...
    Array<String> GetControlPaths(InputValueType InFilter);

    // The device that most recently drove an action active, so callers can tell
    // whether the player is currently on keyboard+mouse or a gamepad. Null until
    // the first input; persists until a different device takes over.
    WeakObjectPtr<InputDevice> GetLastActiveDevice() const;
    void SetLastActiveDevice(const WeakObjectPtr<InputDevice>& InDevice);

    void Tick(float InDeltatime);

private:
    Array<SharedObjectPtr<InputDevice>> m_Devices;
    Array<SharedObjectPtr<InputActionMapping>> m_ActionMappings;
    WeakObjectPtr<InputDevice> m_LastActiveDevice;
};