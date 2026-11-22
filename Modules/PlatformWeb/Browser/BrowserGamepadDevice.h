#pragma once
#include "InputSystem/GamepadDevice.h"
#include "BrowserGamepadDevice.gen.h"

/** Reads the browser's Gamepad API. TODO: Multi gamepad support */
class BrowserGamepadDevice : public GamepadDevice {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Tick() override;
    virtual void ApplyMotorSpeeds(float InLowFrequency, float InHighFrequency) override;

private:
    int32_t m_GamepadIndex = -1;
    float m_LowFrequency = 0.0f;
    float m_HighFrequency = 0.0f;
    double m_LastEffectTime = 0.0;
};
