#include "MaterialDetailsCustomization.h"

#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "Tabs/MajorTab.h"
#include "UI/UIAssetSlot.h"
#include "UI/UIDragNumber.h"
#include "HeroTools/ThumbnailRenderer.h"
#include "Assets/ShaderGraph.h"
#include "Assets/Texture2D.h"

static const char* s_Axes[] = { "X", "Y", "Z", "W" };

static void NotifyEdited(DetailsTab& InTab, Material& InMaterial) {
    if (MajorTab* owner = InTab.GetMajorTab()) {
        owner->OnObjectEdited(&InMaterial);
    }
}

void MaterialDetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    Material* material = Cast<Material>(InObject);
    if (!material) {
        return;
    }

    const WeakObjectPtr<Material> weak(material);

    if (!Cast<ShaderGraph>(material)) {
        DetailsCategory& base = AddCategory(InList, InTab, "Material", 0);
        DetailsRow& row = AddRow(*base.GetBody(), InTab, "Shader Graph", 0);
        row.Height = 52.0f;

        UIAssetSlot* slot = row.GetValueHost()->Add<UIAssetSlot>();
        slot->Fill();
        slot->AssetClass = ShaderGraph::StaticClass();
        slot->Thumbnails = &InTab.GetThumbnails();
        slot->GetAsset = [weak]() -> Asset* {
            Material* current = weak.Get();
            return current ? current->GetBaseGraph() : nullptr;
        };
        slot->SetAsset = [weak, tab = &InTab](Asset* InAsset) {
            Material* current = weak.Get();
            if (!current) {
                return;
            }
            current->SetBaseGraph(Cast<ShaderGraph>(InAsset));
            NotifyEdited(*tab, *current);
            tab->MarkDirty();
        };
        slot->Build();
    }

    if (material->GetInputs().IsEmpty()) {
        return;
    }

    DetailsCategory& parameters = AddCategory(InList, InTab, "Parameters", 0);
    for (const ShaderGraphProperty& input : material->GetInputs()) {
        AddInputRows(*parameters.GetBody(), InTab, *material, input);
    }
}

void MaterialDetailsCustomization::AddInputRows(UINode& InBody, DetailsTab& InTab, Material& InMaterial,
                                                const ShaderGraphProperty& InInput) {
    const WeakObjectPtr<Material> weak(&InMaterial);
    const String name = InInput.Name;

    const auto bindOverride = [weak, name, tab = &InTab](DetailsRow& InRow) {
        InRow.IsOverridden = [weak, name] {
            Material* current = weak.Get();
            return current && current->IsInputOverridden(name);
        };
        InRow.ResetAction = [weak, name, tab] {
            Material* current = weak.Get();
            if (!current) {
                return;
            }
            current->ClearInputOverride(name);
            NotifyEdited(*tab, *current);
        };
    };

    if (InInput.IsTexture()) {
        DetailsRow& row = AddRow(InBody, InTab, PrettyPropertyName(name), 1);
        row.Height = 52.0f;
        bindOverride(row);

        UIAssetSlot* slot = row.GetValueHost()->Add<UIAssetSlot>();
        slot->Fill();
        slot->AssetClass = Texture2D::StaticClass();
        slot->Thumbnails = &InTab.GetThumbnails();
        slot->GetAsset = [weak, name]() -> Asset* {
            Material* current = weak.Get();
            return current ? current->GetInputTexture(name) : nullptr;
        };
        slot->SetAsset = [weak, name, tab = &InTab](Asset* InAsset) {
            Material* current = weak.Get();
            if (!current) {
                return;
            }
            current->SetInputTexture(name, Cast<Texture2D>(InAsset));
            NotifyEdited(*tab, *current);
        };
        slot->Build();
        return;
    }

    const uint32_t components = ShaderValue::GetComponentCount(InInput.Type);
    UINode* host = &InBody;
    int32_t depth = 1;
    if (components > 1) {
        DetailsCategory& category = AddCategory(InBody, InTab, PrettyPropertyName(name), 1);
        host = category.GetBody();
        depth = 2;
    }

    for (uint32_t i = 0; i < components; i++) {
        DetailsRow& row = AddRow(*host, InTab, components == 1 ? PrettyPropertyName(name) : s_Axes[i], depth);
        bindOverride(row);

        UIDragNumber* drag = row.GetValueHost()->Add<UIDragNumber>();
        drag->Fill();
        drag->Get = [weak, name, i]() -> double {
            Material* current = weak.Get();
            return current ? (double)current->GetInputValue(name)[i] : 0.0;
        };
        drag->Set = [weak, name, i, tab = &InTab](double InValue) {
            Material* current = weak.Get();
            if (!current) {
                return;
            }
            Vec4 value = current->GetInputValue(name);
            value[i] = (float)InValue;
            current->SetInputValue(name, value);
            NotifyEdited(*tab, *current);
        };
    }
}
