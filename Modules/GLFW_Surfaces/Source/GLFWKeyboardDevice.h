#pragma once
#include "InputSystem/KeyboardDevice.h"
#include "GLFWKeyboardDevice.gen.h"

class GLFWKeyboardDevice : public KeyboardDevice {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Tick() override;
};