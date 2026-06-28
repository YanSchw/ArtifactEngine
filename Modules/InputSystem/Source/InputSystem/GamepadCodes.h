#pragma once
#include "Object/Enum.h"
#include "GamepadCodes.gen.h"

ARTIFACT_ENUM();
enum class GamepadCode : uint16_t {
    // From glfw3.h (GLFW_GAMEPAD_BUTTON_*), layout-neutral face-button naming.
    // South/East/West/North = A/B/X/Y (Xbox) = Cross/Circle/Square/Triangle (PS).
    South = 0,  // bottom face button (A / Cross)
    East = 1,   // right face button  (B / Circle)
    West = 2,   // left face button   (X / Square)
    North = 3,  // top face button    (Y / Triangle)
    LeftBumper = 4,
    RightBumper = 5,
    Back = 6,
    Start = 7,
    Guide = 8,
    LeftThumb = 9,
    RightThumb = 10,
    DPadUp = 11,
    DPadRight = 12,
    DPadDown = 13,
    DPadLeft = 14
};
