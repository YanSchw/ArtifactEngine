#pragma once
#include "InputDevice.h"
#include "MouseCodes.h"
#include "Common/Types.h"
#include "MouseDevice.gen.h"

class MouseDevice : public InputDevice {
public:
    ARTIFACT_CLASS();

    MouseDevice();

    // Buttons
    bool IsPressed(MouseCode InCode);
    bool IsDown(MouseCode InCode);
    bool IsUp(MouseCode InCode);

    // Position (in pixels, relative to the top-left of the window)
    Vec2 GetPosition() const;
    float GetX() const;
    float GetY() const;

    // Movement since the previous frame
    Vec2 GetPositionDelta() const;
    float GetDeltaX() const;
    float GetDeltaY() const;

    // Scroll wheel movement during this frame (X = horizontal, Y = vertical)
    Vec2 GetScrollDelta() const;
    float GetScrollX() const;
    float GetScrollY() const;

    // InputDevice / control-path interface
    virtual String GetDeviceName() const override;
    virtual Array<InputControl> GetControls() const override;
    virtual bool ReadControl(const String& InControl, InputValue& OutValue) override;

    // Return the first available instance of MouseDevice if present
    static MouseDevice* Instance();

protected:
    virtual void Tick() override;

protected:
    Map<MouseCode, bool> m_Buttons;
    Vec2 m_Position = {0.0f, 0.0f};
    Vec2 m_ScrollDelta = {0.0f, 0.0f};

private:
    Map<MouseCode, bool> m_ButtonsLastFrame;
    Vec2 m_PositionLastFrame = {0.0f, 0.0f};
    Map<String, MouseCode> m_NameToButton;
};
