#include "InputActionMap.h"

InputAction* InputActionMap::Find(const String& InName) const {
    for (const auto& action : Actions) {
        if (action->Name == InName) {
            return action.Get();
        }
    }
    return nullptr;
}

void InputActionMap::Evaluate() {
    if (!Enabled) {
        return;
    }
    for (const auto& action : Actions) {
        action->Evaluate();
    }
}
