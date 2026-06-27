#pragma once
#include "Object/Object.h"
#include "InputDevice.gen.h"

class InputDevice : public Object {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Tick() = 0;

    friend class InputSystem;
};