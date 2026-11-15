#pragma once
#include "AssetDetailsCustomization.h"
#include "SceneDetailsCustomization.gen.h"

class SceneDetailsCustomization : public AssetDetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override;

protected:
    virtual bool WantsClassCategory(const Class& InClass) const override;
};
