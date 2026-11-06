#include "MeshDetailsCustomization.h"

#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "UI/UIAssetSlot.h"
#include "Assets/Material.h"

String MeshDetailsCustomization::GetSlotLabel(const Mesh& InMesh, int32_t InSlot) {
    const String& name = InMesh.GetSubMeshes()[InSlot].Name;
    return name.empty() ? "Slot " + std::to_string(InSlot) : name;
}

DetailsRow& MeshDetailsCustomization::AddMaterialSlotRow(UINode& InBody, DetailsTab& InTab, const String& InLabel,
                                                         std::function<Asset*()> InGet, std::function<void(Asset*)> InSet) {
    DetailsRow& row = AddRow(InBody, InTab, InLabel, 0);
    row.Height = 52.0f;

    UIAssetSlot* slot = row.GetValueHost()->Add<UIAssetSlot>();
    slot->Fill();
    slot->AssetClass = Material::StaticClass();
    slot->Thumbnails = &InTab.GetThumbnails();
    slot->GetAsset = std::move(InGet);
    slot->SetAsset = std::move(InSet);
    slot->Build();
    return row;
}

void MeshDetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    Mesh* mesh = Cast<Mesh>(InObject);
    if (!mesh) {
        return;
    }
    if (!mesh->IsSubAsset()) {
        AssetDetailsCustomization::BuildContent(InList, InObject, InTab);
    }
    if (mesh->GetSubMeshes().IsEmpty()) {
        return;
    }

    const WeakObjectPtr<Mesh> weak(mesh);
    DetailsCategory& slots = AddCategory(InList, InTab, "Material Slots", 0);
    for (int32_t slot = 0; slot < mesh->GetSubMeshes().Size(); slot++) {
        AddMaterialSlotRow(*slots.GetBody(), InTab, GetSlotLabel(*mesh, slot),
            [weak, slot]() -> Asset* {
                Mesh* current = weak.Get();
                return current ? current->GetAssignedMaterial(slot) : nullptr;
            },
            [weak, slot, tab = &InTab](Asset* InAsset) {
                if (Mesh* current = weak.Get()) {
                    tab->RecordEdit("Edit Material Slot", current);
                    current->SetMaterial(slot, Cast<Material>(InAsset));
                    NotifyPropertyEdited(tab, current, "m_Materials");
                }
            });
    }
}

void MeshDetailsCustomization::BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) {
    WeakObjectPtr<Object> weak(InObject);
    for (Property* property : Property::GetTypeProperties(InClass.Name)) {
        if (property->Name != "m_Materials" && property->Name != "m_SourceMesh") {
            AddPropertyRow(*InCategory.GetBody(), InTab, weak, 0, property, 0);
        }
    }
}
