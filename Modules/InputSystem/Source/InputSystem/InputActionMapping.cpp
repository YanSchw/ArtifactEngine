#include "InputActionMapping.h"
#include "Assets/AssetManager.h"

InputActionMapping* InputActionMapping::CreateEmpty(const String& InDirectory, const String& InName) {
    InputActionMapping* mapping = Cast<InputActionMapping>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!mapping) {
        return nullptr;
    }

    mapping->Name = InName;
    AssetManager::Get().SaveAsset(mapping);
    return mapping;
}

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

String InputActionMapping::GetDisplayName() const {
    const String path = AssetManager::Get().GetAssetPath(GetId());
    return path.empty() ? Asset::GetDisplayName() : DisplayNameFromPath(path);
}

void InputActionMapping::Load() {
    // Actions and bindings are populated from the asset's PROPERTY fields on
    // deserialization, so there is nothing extra to stream in here.
}

void InputActionMapping::Unload() {
    Actions.Clear();
}
