#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "InputAction.h"
#include "InputActionMap.gen.h"

class InputActionMap : public Object {
public:
    ARTIFACT_CLASS();

    String Name;
    bool   Enabled = true;
    Array<SharedObjectPtr<InputAction>> Actions;

    // Find an action by name, or nullptr if absent.
    InputAction* Find(const String& InName) const;

    // Evaluate every action (no-op while disabled).
    void Evaluate();
};
