#pragma once
#include "Object/Enum.h"
#include "Object/Pointer.h"
#include "Common/Types.h"
#include "InputValue.gen.h"

class InputDevice;

ARTIFACT_ENUM();
enum class InputValueType : uint8_t {
    Bool,
    Float,
    Vec2
};

// Unified value produced by any binding or device control. A button lives in
// x (0 or 1), a 1D axis in x ([-1, 1]), and a 2D control in the full vector.
// Device / DeviceIndex identify which device produced the value (so a callback
// can tell which gamepad fired); they stay unset for composite/synthetic values.
struct InputValue {
    Vec2 Raw = {0.0f, 0.0f};

    // Source device, or unresolved reads.
    WeakObjectPtr<InputDevice> Device;
    // Index of the source device within InputSystem, or -1 if none.
    int32_t DeviceIndex = -1;

    bool  AsBool()  const { return Raw.x != 0.0f; }
    float AsFloat() const { return Raw.x; }
    Vec2  AsVec2()  const { return Raw; }
    float Magnitude() const { return glm::length(Raw); }
};
