#pragma once
#include "InputSystem/GamepadDevice.h"
#include "BrowserGamepadDevice.gen.h"

/** Reads the browser's Gamepad API. TODO: Multi gamepad support */
class BrowserGamepadDevice : public GamepadDevice {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Tick() override;
};
