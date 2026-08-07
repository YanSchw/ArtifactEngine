#pragma once
#include "DetailsCustomization.h"
#include "Assets/Asset.h"
#include "AssetDetailsCustomization.gen.h"

/** Inspects any asset by its own properties; the identity Asset itself carries stays hidden. */
class AssetDetailsCustomization : public DetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return Asset::StaticClass(); }

protected:
    virtual bool WantsClassCategory(const Class& InClass) const override {
        return InClass != Asset::StaticClass() && DetailsCustomization::WantsClassCategory(InClass);
    }
};
