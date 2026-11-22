#include "MacOSGamepadBridge.h"

#import <GameController/GameController.h>

bool ReadMacOSGamepad(MacOSGamepadState& OutState) {
    GCExtendedGamepad* pad = nil;
    for (GCController* controller in GCController.controllers) {
        if (controller.extendedGamepad) {
            pad = controller.extendedGamepad;
            break;
        }
    }
    if (!pad) {
        return false;
    }

    // Face buttons by position: South/East/West/North = A/B/X/Y = Cross/Circle/Square/Triangle.
    GCControllerButtonInput* buttons[15] = {
        pad.buttonA, pad.buttonB, pad.buttonX, pad.buttonY,
        pad.leftShoulder, pad.rightShoulder, pad.buttonOptions, pad.buttonMenu,
        nil, pad.leftThumbstickButton, pad.rightThumbstickButton,
        pad.dpad.up, pad.dpad.right, pad.dpad.down, pad.dpad.left,
    };
    if (@available(macOS 11.0, *)) {
        buttons[8] = pad.buttonHome;
    }
    for (int i = 0; i < 15; i++) {
        OutState.Buttons[i] = buttons[i] && buttons[i].isPressed;
    }

    // GameController already reports stick Y up-positive.
    OutState.LeftStick[0] = pad.leftThumbstick.xAxis.value;
    OutState.LeftStick[1] = pad.leftThumbstick.yAxis.value;
    OutState.RightStick[0] = pad.rightThumbstick.xAxis.value;
    OutState.RightStick[1] = pad.rightThumbstick.yAxis.value;
    OutState.LeftTrigger = pad.leftTrigger.value;
    OutState.RightTrigger = pad.rightTrigger.value;
    return true;
}
