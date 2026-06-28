#include "InputActionMapping.h"

InputAction* InputActionMapping::Find(const String& InName) const {
    for (const auto& action : Actions) {
        if (action->Name == InName) {
            return action.Get();
        }
    }
    return nullptr;
}

void InputActionMapping::Evaluate() {
    if (!Enabled) {
        return;
    }
    for (const auto& action : Actions) {
        action->Evaluate();
    }
}

void InputActionMapping::Load() {
    // Actions and bindings are populated from the asset's PROPERTY fields on
    // deserialization, so there is nothing extra to stream in here.
}

void InputActionMapping::Unload() {
    Actions.Clear();
}
