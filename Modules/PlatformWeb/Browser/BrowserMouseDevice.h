#pragma once
#include "InputSystem/MouseDevice.h"
#include "BrowserMouseDevice.gen.h"

class BrowserMouseDevice : public MouseDevice {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Tick() override;
};
