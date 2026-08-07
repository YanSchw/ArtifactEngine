#pragma once
#include "DetailsCustomization.h"
#include "Assets/Material.h"
#include "MaterialDetailsCustomization.gen.h"

/** Lists the inputs the base shader graph exposes, each row overriding one of them. */
class MaterialDetailsCustomization : public DetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return Material::StaticClass(); }
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) override;

private:
    void AddInputRows(UINode& InBody, DetailsTab& InTab, Material& InMaterial, const ShaderGraphProperty& InInput);
};
