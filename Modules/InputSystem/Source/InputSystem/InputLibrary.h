#pragma once
#include "Object/Object.h"
#include "Common/Types.h"
#include "KeyCodes.h"
#include "MouseCodes.h"
#include "InputLibrary.gen.h"

class InputAction;

class InputLibrary : public Object {
public:
    ARTIFACT_CLASS();

    FUNCTION(Pure, Category = "Input|Keyboard")
    static bool IsKeyHeld(KeyCode InKey);
    FUNCTION(Pure, Category = "Input|Keyboard")
    static bool WasKeyPressed(KeyCode InKey);
    FUNCTION(Pure, Category = "Input|Keyboard")
    static bool WasKeyReleased(KeyCode InKey);

    FUNCTION(Pure, Category = "Input|Mouse")
    static bool IsMouseButtonHeld(MouseCode InButton);
    FUNCTION(Pure, Category = "Input|Mouse")
    static bool WasMouseButtonPressed(MouseCode InButton);
    FUNCTION(Pure, Category = "Input|Mouse")
    static Vec2 GetMousePosition();
    FUNCTION(Pure, Category = "Input|Mouse")
    static Vec2 GetMouseDelta();

    FUNCTION(Pure, Category = "Input|Touch")
    static bool IsTouching();
    FUNCTION(Pure, Category = "Input|Touch")
    static int32_t GetTouchCount();
    FUNCTION(Pure, Category = "Input|Touch")
    static Vec2 GetTouchPosition(int32_t InIndex);
    FUNCTION(Pure, Category = "Input|Touch")
    static Vec2 GetTouchDelta(int32_t InIndex);

    FUNCTION(Pure, Category = "Input|Actions")
    static InputAction* FindAction(const String& InName);
    FUNCTION(Pure, Category = "Input|Actions")
    static bool IsActionActive(const String& InName);
    FUNCTION(Pure, Category = "Input|Actions")
    static bool WasActionPerformed(const String& InName);
    FUNCTION(Pure, Category = "Input|Actions")
    static bool WasActionReleased(const String& InName);
    FUNCTION(Pure, Category = "Input|Actions")
    static float ReadActionFloat(const String& InName);
    FUNCTION(Pure, Category = "Input|Actions")
    static Vec2 ReadActionVec2(const String& InName);
};
