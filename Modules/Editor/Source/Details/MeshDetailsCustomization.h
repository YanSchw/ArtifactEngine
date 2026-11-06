#pragma once
#include "AssetDetailsCustomization.h"
#include "Assets/Mesh.h"
#include <functional>
#include "MeshDetailsCustomization.gen.h"

class DetailsRow;

/** A material row per slot. Inside a collection the import settings belong to the collection, so
 *  only the slots are shown there. */
class MeshDetailsCustomization : public AssetDetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return Mesh::StaticClass(); }
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) override;

    static String GetSlotLabel(const Mesh& InMesh, int32_t InSlot);
    static DetailsRow& AddMaterialSlotRow(UINode& InBody, DetailsTab& InTab, const String& InLabel,
                                          std::function<Asset*()> InGet, std::function<void(Asset*)> InSet);

protected:
    virtual void BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) override;
};
