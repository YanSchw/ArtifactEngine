#pragma once
#include "InputSystem/MouseDevice.h"
#include "GLFWMouseDevice.gen.h"

class GLFWMouseDevice : public MouseDevice {
public:
    ARTIFACT_CLASS();

    GLFWMouseDevice();

protected:
    virtual void Tick() override;
};
