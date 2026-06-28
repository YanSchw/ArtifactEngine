#include "InputBinding.h"
#include "InputSystem.h"

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
    return {{positive.AsFloat() - negative.AsFloat(), 0.0f}};
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
    return {value};
}
