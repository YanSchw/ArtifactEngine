#pragma once
#include "Object/Object.h"
#include "InputValue.h"
#include "InputBinding.gen.h"

class InputBinding : public Object {
public:
    ARTIFACT_CLASS();

    virtual InputValue Read() const = 0;
};

// The single device-agnostic leaf: addresses one control by path,
// e.g. "Keyboard/W", "Mouse/Delta", "Gamepad/North".
class PathBinding : public InputBinding {
public:
    ARTIFACT_CLASS();

    String Path;

    virtual InputValue Read() const override;
};

// float built from two opposing control paths.
class Axis1DComposite : public InputBinding {
public:
    ARTIFACT_CLASS();

    String Negative;
    String Positive;

    virtual InputValue Read() const override;
};

// Vec2 built from four directional control paths (e.g. WASD).
class Vector2Composite : public InputBinding {
public:
    ARTIFACT_CLASS();

    String Up;
    String Down;
    String Left;
    String Right;
    bool Normalize = true;

    virtual InputValue Read() const override;
};
