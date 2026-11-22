#pragma once

// Plain bridge between the engine and MacOSGamepadBridge.mm. Engine headers can't be included from
// Objective-C++: the reflection type `Class` clashes with the Objective-C runtime's `Class`.

struct MacOSGamepadState {
    // Indexed like GamepadCode: South, East, West, North, LeftBumper, RightBumper, Back, Start,
    // Guide, LeftThumb, RightThumb, DPadUp, DPadRight, DPadDown, DPadLeft.
    bool Buttons[15] = {};
    float LeftStick[2] = {};   // x right, y up
    float RightStick[2] = {};
    float LeftTrigger = 0.0f;  // 0..1
    float RightTrigger = 0.0f;
};

/** Reads the first connected controller with an extended gamepad profile through the GameController
 *  framework. False when none is connected. */
bool ReadMacOSGamepad(MacOSGamepadState& OutState);
