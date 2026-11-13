#pragma once
#include "InputSystem/MultiTouchDevice.h"
#include "iOSMultiTouchDevice.gen.h"

class iOSMultiTouchDevice : public MultiTouchDevice {
public:
    ARTIFACT_CLASS();

protected:
    virtual void PollTouches() override;
};
