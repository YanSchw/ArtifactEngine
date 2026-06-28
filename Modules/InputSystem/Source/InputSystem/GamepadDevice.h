#pragma once
#include "InputDevice.h"
#include "GamepadCodes.h"
#include "Common/Types.h"
#include "GamepadDevice.gen.h"

class GamepadDevice : public InputDevice {
public:
    ARTIFACT_CLASS();

    GamepadDevice();

    // Buttons
    bool IsPressed(GamepadCode InCode);
    bool IsDown(GamepadCode InCode);
    bool IsUp(GamepadCode InCode);

    // Sticks (each axis in [-1, 1]) and triggers (in [0, 1])
    Vec2 GetLeftStick() const;
    Vec2 GetRightStick() const;
    float GetLeftTrigger() const;
    float GetRightTrigger() const;

    // D-pad as a Vec2 derived from its four buttons (x = right, y = up)
    Vec2 GetDPad();

    // Whether the underlying physical pad is currently connected
    bool IsConnected() const;

    // InputDevice / control-path interface
    virtual String GetDeviceName() const override;
    virtual Array<InputControl> GetControls() const override;
    virtual bool ReadControl(const String& InControl, InputValue& OutValue) override;

    // Return the first available instance of GamepadDevice if present
    static GamepadDevice* Instance();

protected:
    virtual void Tick() override;

protected:
    Map<GamepadCode, bool> m_Buttons;
    Vec2 m_LeftStick = {0.0f, 0.0f};
    Vec2 m_RightStick = {0.0f, 0.0f};
    float m_LeftTrigger = 0.0f;
    float m_RightTrigger = 0.0f;
    bool m_Connected = false;

private:
    Map<GamepadCode, bool> m_ButtonsLastFrame;
    Map<String, GamepadCode> m_NameToButton;
};
