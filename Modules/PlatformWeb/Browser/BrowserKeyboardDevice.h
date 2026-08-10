#pragma once
#include "InputSystem/KeyboardDevice.h"
#include "BrowserKeyboardDevice.gen.h"

class BrowserKeyboardDevice : public KeyboardDevice {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Tick() override;
};
