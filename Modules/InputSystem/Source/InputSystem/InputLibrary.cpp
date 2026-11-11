#include "InputLibrary.h"
#include "InputSystem.h"
#include "InputAction.h"
#include "InputActionMapping.h"
#include "KeyboardDevice.h"
#include "MouseDevice.h"

bool InputLibrary::IsKeyHeld(KeyCode InKey) {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && keyboard->IsPressed(InKey);
}

bool InputLibrary::WasKeyPressed(KeyCode InKey) {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && keyboard->IsDown(InKey);
}

bool InputLibrary::WasKeyReleased(KeyCode InKey) {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && keyboard->IsUp(InKey);
}

bool InputLibrary::IsMouseButtonHeld(MouseCode InButton) {
    MouseDevice* mouse = MouseDevice::Instance();
    return mouse && mouse->IsPressed(InButton);
}

bool InputLibrary::WasMouseButtonPressed(MouseCode InButton) {
    MouseDevice* mouse = MouseDevice::Instance();
    return mouse && mouse->IsDown(InButton);
}

Vec2 InputLibrary::GetMousePosition() {
    MouseDevice* mouse = MouseDevice::Instance();
    return mouse ? mouse->GetPosition() : Vec2(0.0f);
}

Vec2 InputLibrary::GetMouseDelta() {
    MouseDevice* mouse = MouseDevice::Instance();
    return mouse ? mouse->GetPositionDelta() : Vec2(0.0f);
}

InputAction* InputLibrary::FindAction(const String& InName) {
    for (const SharedObjectPtr<InputActionMapping>& mapping : InputSystem::Get().GetActionMappings()) {
        if (InputAction* action = mapping ? mapping->Find(InName) : nullptr) {
            return action;
        }
    }
    return nullptr;
}

bool InputLibrary::IsActionActive(const String& InName) {
    InputAction* action = FindAction(InName);
    return action && action->IsActive();
}

bool InputLibrary::WasActionPerformed(const String& InName) {
    InputAction* action = FindAction(InName);
    return action && action->WasPerformedThisFrame();
}

bool InputLibrary::WasActionReleased(const String& InName) {
    InputAction* action = FindAction(InName);
    return action && action->WasReleasedThisFrame();
}

float InputLibrary::ReadActionFloat(const String& InName) {
    InputAction* action = FindAction(InName);
    return action ? action->ReadFloat() : 0.0f;
}

Vec2 InputLibrary::ReadActionVec2(const String& InName) {
    InputAction* action = FindAction(InName);
    return action ? action->ReadVec2() : Vec2(0.0f);
}
