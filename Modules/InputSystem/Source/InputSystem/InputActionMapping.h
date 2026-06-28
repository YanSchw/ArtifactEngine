#pragma once
#include "Assets/Asset.h"
#include "Object/Pointer.h"
#include "InputAction.h"
#include "InputActionMapping.gen.h"

// A named, enable-able set of input actions, authored and loaded as an Asset
class InputActionMapping : public Asset {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    String Name;

    PROPERTY()
    bool Enabled = true;

    PROPERTY()
    Array<SharedObjectPtr<InputAction>> Actions;

    // Find an action by name, or nullptr if absent.
    InputAction* Find(const String& InName) const;

    // Evaluate every action (no-op while disabled).
    void Evaluate();

protected:
    virtual void Load() override;
    virtual void Unload() override;
};
