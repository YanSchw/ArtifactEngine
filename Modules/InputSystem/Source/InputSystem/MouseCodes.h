#pragma once
#include "Object/Enum.h"
#include "MouseCodes.gen.h"

ARTIFACT_ENUM();
enum class MouseCode : uint16_t {
    // From glfw3.h
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Button6 = 5,
    Button7 = 6,
    Button8 = 7
};
