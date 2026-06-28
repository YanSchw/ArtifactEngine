#pragma once
#include "InputDevice.h"
#include "KeyCodes.h"
#include "KeyboardDevice.gen.h"

class KeyboardDevice : public InputDevice {
public:
    ARTIFACT_CLASS();

    KeyboardDevice();

    bool IsPressed(KeyCode InCode);
    bool IsDown(KeyCode InCode);
    bool IsUp(KeyCode InCode);

    // InputDevice / control-path interface
    virtual String GetDeviceName() const override;
    virtual Array<InputControl> GetControls() const override;
    virtual bool ReadControl(const String& InControl, InputValue& OutValue) override;

    // Return the first available instance of KeyboardDevice if present
    static KeyboardDevice* Instance();

protected:
    virtual void Tick() override;

protected:
    Map<KeyCode, bool> m_Keys;

private:
    Map<KeyCode, bool> m_KeysLastFrame;
    Map<String, KeyCode> m_NameToKey;
};
