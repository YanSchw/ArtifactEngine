#pragma once
#include "Core/EngineSubsystem.h"
#include "InputSystem/GamepadDevice.h"
#include "MacOSGamepad.gen.h"

/** The macOS gamepad, read through Apple's GameController framework. */
class GameControllerGamepadDevice : public GamepadDevice {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Tick() override;
};

/** Adds the GameController gamepad to the InputSystem when the engine starts. */
class MacOSGamepadSubsystem : public EngineSubsystem {
public:
    ARTIFACT_CLASS();

    virtual void Initialize() override;
};
