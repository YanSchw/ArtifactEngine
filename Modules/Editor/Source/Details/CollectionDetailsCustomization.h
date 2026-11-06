#pragma once
#include "AssetDetailsCustomization.h"
#include "Assets/CollectionAsset.h"
#include "CollectionDetailsCustomization.gen.h"

/** The import settings as one category per kind of sub-asset, the shader template picked from the
 *  templates that exist. */
class CollectionDetailsCustomization : public AssetDetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return CollectionAsset::StaticClass(); }

protected:
    virtual void BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) override;

private:
    static void AddTemplateRow(UINode& InBody, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, Property* InRootProperty);
};
