#pragma once
#include "InputSystem/GamepadDevice.h"
#include "VirtualGamepadDevice.gen.h"

struct VirtualGamepadState {
    uint32_t Buttons = 0;
    Vec2 LeftStick = Vec2(0.0f);
    Vec2 RightStick = Vec2(0.0f);
    float LeftTrigger = 0.0f;
    float RightTrigger = 0.0f;

    void SetButton(GamepadCode InCode, bool InPressed);
    bool IsButtonPressed(GamepadCode InCode) const;
};

class VirtualGamepadDevice : public GamepadDevice {
public:
    ARTIFACT_CLASS();

    void Submit(const VirtualGamepadState& InState);

protected:
    virtual void Tick() override;

private:
    VirtualGamepadState m_Pending;
    bool m_HasPending = false;
};
