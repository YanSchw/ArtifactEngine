#include "SceneDetailsCustomization.h"
#include "Assets/Scene.h"

Class SceneDetailsCustomization::GetSupportedClass() const {
    return Scene::StaticClass();
}

bool SceneDetailsCustomization::WantsClassCategory(const Class& InClass) const {
    return InClass != NodeAsset::StaticClass() && AssetDetailsCustomization::WantsClassCategory(InClass);
}
