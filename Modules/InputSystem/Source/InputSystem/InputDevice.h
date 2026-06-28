#pragma once
#include "Object/Object.h"
#include "InputValue.h"
#include "InputDevice.gen.h"

// A single addressable control on a device, used to build editor paths.
struct InputControl {
    String         Name;   // "W", "Left", "Delta"
    InputValueType Type = InputValueType::Bool;
};

class InputDevice : public Object {
public:
    ARTIFACT_CLASS();

    // Path prefix used to address this device, e.g. "Keyboard".
    virtual String GetDeviceName() const = 0;

    // Every addressable control on this device (for editor enumeration).
    virtual Array<InputControl> GetControls() const = 0;

    // Read a control by name; returns false if this device has no such control.
    virtual bool ReadControl(const String& InControl, InputValue& OutValue) = 0;

protected:
    virtual void Tick() = 0;

    friend class InputSystem;
};
