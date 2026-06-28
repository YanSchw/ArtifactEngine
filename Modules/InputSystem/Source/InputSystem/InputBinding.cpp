#include "InputBinding.h"
#include "InputSystem.h"
#include <cmath>

InputValue PathBinding::Read() const {
    InputValue value;
    InputSystem::Get().ReadPath(Path, value);
    return value;
}

InputValue Axis1DComposite::Read() const {
    InputSystem& input = InputSystem::Get();
    InputValue negative, positive;
    input.ReadPath(Negative, negative);
    input.ReadPath(Positive, positive);

    InputValue result;
    result.Raw = {positive.AsFloat() - negative.AsFloat(), 0.0f};
    // Carry the source from the stronger-actuated side.
    const InputValue& dominant =
        std::abs(positive.AsFloat()) >= std::abs(negative.AsFloat()) ? positive : negative;
    result.Device = dominant.Device;
    result.DeviceIndex = dominant.DeviceIndex;
    return result;
}

InputValue Vector2Composite::Read() const {
    InputSystem& input = InputSystem::Get();
    InputValue up, down, left, right;
    input.ReadPath(Up, up);
    input.ReadPath(Down, down);
    input.ReadPath(Left, left);
    input.ReadPath(Right, right);

    Vec2 value = {right.AsFloat() - left.AsFloat(), up.AsFloat() - down.AsFloat()};
    if (Normalize && (value.x != 0.0f || value.y != 0.0f)) {
        value = glm::normalize(value);
    }

    InputValue result;
    result.Raw = value;
    // Carry the source from the strongest-actuated component.
    const InputValue* dominant = &up;
    for (const InputValue* component : {&down, &left, &right}) {
        if (std::abs(component->AsFloat()) > std::abs(dominant->AsFloat())) {
            dominant = component;
        }
    }
    result.Device = dominant->Device;
    result.DeviceIndex = dominant->DeviceIndex;
    return result;
}
