#include "NodeDetailsCustomization.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "UI/UIDragNumber.h"
#include "UI/UICheckbox.h"
#include "MeshDetailsCustomization.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/StaticMeshNode.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIHStack.h"
#include "Assets/AssetManager.h"
#include "Assets/Material.h"
#include "Assets/Mesh.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"
#include <cmath>
#include <memory>

float NodeDetailsCustomization::BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) {
    Node* node = Cast<Node>(InObject);
    WeakObjectPtr<Node> weak(node);
    MajorTab* major = InTab.GetMajorTab();
    const bool isAssetRoot = major && major->IsAssetRootNode(InObject);

    UIQuad* background = InHeader.Add<UIQuad>();
    background->Fill();
    background->Color = EditorStyle::ToolBar;

    UISvg* icon = background->Add<UISvg>();
    icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
    icon->Position = Vec2(10.0f, 0.0f);
    icon->Size = Vec2(22.0f, 22.0f);
    icon->Tint = EditorStyle::Text;
    icon->Image = EditorIcons::GetNodeIcon(node->GetClass());

    UICheckbox* enabled = background->Add<UICheckbox>();
    enabled->Anchor = enabled->Pivot = Vec2(1.0f, 0.5f);
    enabled->Position = Vec2(-10.0f, 0.0f);
    enabled->Bind = [enabled, weak] {
        if (Node* bound = weak.Get()) {
            enabled->IsOn = bound->IsSelfEnabled();
        }
    };
    enabled->Changed = [weak, tab = &InTab](bool InValue) {
        if (Node* bound = weak.Get()) {
            tab->RecordEdit("Edit Enabled", bound);
            bound->SetEnabled(InValue);
            DetailsCustomization::NotifyPropertyEdited(tab, bound, "m_Enabled");
        }
    };

    UITextArea* name = background->Add<UITextArea>();
    name->Anchor = name->Pivot = Vec2(0.0f, 0.5f);
    name->Position = Vec2(42.0f, 0.0f);
    name->Size = { 1.0_rel - 84.0_px, 24.0f };
    name->SingleLine = true;
    name->FontSize = EditorStyle::FontSize;
    name->TextColor = EditorStyle::TextBright;
    name->CaretColor = EditorStyle::TextBright;
    name->BackgroundColor = EditorStyle::PanelDark;
    name->FocusedBorderColor = EditorStyle::Accent;
    name->CornerRadius = 2.0f;
    name->Padding = UIPadding(6.0f, 0.0f);
    name->Bind = [name, weak] {
        if (!name->IsFocused()) {
            if (Node* bound = weak.Get()) {
                name->Text = bound->GetName();
            }
        }
    };

    if (isAssetRoot) {
        name->Interactable = false;
        name->TextColor = EditorStyle::TextDim;
        name->BackgroundColor = EditorStyle::ToolBar;
        return 44.0f;
    }

    const auto commitName = [weak, tab = &InTab](const String& InName) {
        if (Node* bound = weak.Get()) {
            if (!InName.empty()) {
                tab->RecordEdit("Rename", bound);
                bound->SetName(InName);
            }
        }
    };
    name->Submitted = commitName;
    name->FocusLost = [name, commitName] { commitName(name->Text); };

    return 44.0f;
}

bool NodeDetailsCustomization::WantsClassCategory(const Class& InClass) const {
    if (InClass == Node::StaticClass()) {
        return false;
    }
    return InClass == Node3D::StaticClass() || DetailsCustomization::WantsClassCategory(InClass);
}

void NodeDetailsCustomization::BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) {
    if (InClass == Node3D::StaticClass()) {
        if (Node3D* node = Cast<Node3D>(InObject)) {
            BuildTransformCategory(InCategory, node, InTab);
        }
        return;
    }
    if (InClass == StaticMeshNode::StaticClass()) {
        BuildStaticMeshCategory(InCategory, Cast<StaticMeshNode>(InObject), InTab);
        return;
    }
    DetailsCustomization::BuildClassCategory(InCategory, InClass, InObject, InTab);
}

void NodeDetailsCustomization::BuildStaticMeshCategory(DetailsCategory& InCategory, StaticMeshNode* InNode, DetailsTab& InTab) {
    const WeakObjectPtr<Object> weak(InNode);
    for (Property* property : Property::GetTypeProperties(StaticMeshNode::StaticClass().Name)) {
        if (property->Name != "m_MaterialOverrides") {
            AddPropertyRow(*InCategory.GetBody(), InTab, weak, 0, property, 0);
        }
    }

    Mesh* mesh = InNode ? InNode->GetMesh() : nullptr;
    if (!mesh || mesh->GetSubMeshes().IsEmpty()) {
        return;
    }

    // Each row shows what the slot draws with; picking another material overrides just that slot.
    const WeakObjectPtr<StaticMeshNode> node(InNode);
    DetailsCategory& slots = AddCategory(*InCategory.GetBody(), InTab, "Materials", 1);
    for (int32_t slot = 0; slot < mesh->GetSubMeshes().Size(); slot++) {
        const auto setOverride = [node, slot, tab = &InTab](Material* InMaterial) {
            if (StaticMeshNode* current = node.Get()) {
                tab->RecordEdit("Edit Material Override", current);
                current->SetMaterialOverride(slot, InMaterial);
                NotifyPropertyEdited(tab, current, "m_MaterialOverrides");
            }
        };
        DetailsRow& row = MeshDetailsCustomization::AddMaterialSlotRow(*slots.GetBody(), InTab, MeshDetailsCustomization::GetSlotLabel(*mesh, slot),
            [node, slot]() -> Asset* {
                StaticMeshNode* current = node.Get();
                return current ? current->GetMaterial(slot) : nullptr;
            },
            [node, slot, setOverride](Asset* InAsset) {
                StaticMeshNode* current = node.Get();
                Mesh* currentMesh = current ? current->GetMesh() : nullptr;
                Material* material = Cast<Material>(InAsset);
                setOverride(currentMesh && material == currentMesh->GetMaterial(slot) ? nullptr : material);
            });
        row.IsOverridden = [node, slot] {
            StaticMeshNode* current = node.Get();
            return current && current->GetMaterialOverride(slot) != nullptr;
        };
        row.ResetAction = [setOverride] { setOverride(nullptr); };
    }
}

static void AddVectorRow(UINode& InBody, DetailsTab& InTab, const String& InLabel,
                         const WeakObjectPtr<Node>& InNode, const String& InPropertyName,
                         std::function<Vec3()> InGet, std::function<void(const Vec3&)> InSet,
                         double InSensitivity, float InAxisDefault) {
    struct Axis {
        const char* Name;
        Vec4 Color;
    };
    static const Axis s_Axes[3] = {
        { "X", EditorStyle::TransformX },
        { "Y", EditorStyle::TransformY },
        { "Z", EditorStyle::TransformZ },
    };

    DetailsRow* row = InBody.Add<DetailsRow>();
    row->OwnerTab = &InTab;
    row->SetLabel(InLabel);
    DetailsCustomization::BindOverride(*row, InNode, InPropertyName);

    const auto apply = [InSet, InNode, InPropertyName, InLabel, tab = &InTab](const Vec3& InValue) {
        Node* node = InNode.Get();
        if (!node) {
            return;
        }
        tab->RecordEdit("Edit " + InLabel, node);
        InSet(InValue);
        DetailsCustomization::NotifyPropertyEdited(tab, node, InPropertyName);
    };

    UIHStack* stack = row->GetValueHost()->Add<UIHStack>();
    stack->Fill();
    stack->Gap = 2.0f;

    for (int axis = 0; axis < 3; axis++) {
        UIDragNumber* drag = stack->Add<UIDragNumber>();
        drag->Size = { UIValue::Rel(1.0f / 3.0f), 1.0_rel };
        drag->Prefix = s_Axes[axis].Name;
        drag->PrefixColor = s_Axes[axis].Color;
        drag->Sensitivity = InSensitivity;
        drag->Get = [InGet, axis]() -> double { return (double)InGet()[axis]; };
        drag->Set = [InGet, apply, axis](double InValue) {
            Vec3 value = InGet();
            value[axis] = (float)InValue;
            apply(value);
        };
        drag->PrefixClicked = [InGet, apply, axis, InAxisDefault] {
            Vec3 value = InGet();
            value[axis] = InAxisDefault;
            apply(value);
        };
    }
}

/** Euler angles the user is authoring. A node stores its rotation as a quaternion, and converting
 *  back yields a canonical triple, so re-reading it every frame would snap the other two axes
 *  whenever a drag crosses 180 degrees or gimbal lock. */
struct EulerEditState {
    Vec3 Angles = Vec3(0.0f);
    Quat Applied = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    bool Valid = false;
};

static bool IsSameRotation(const Quat& InA, const Quat& InB) {
    // q and -q are the same rotation, hence the absolute value.
    return std::abs(glm::dot(InA, InB)) > 0.9999995f;
}

void NodeDetailsCustomization::BuildTransformCategory(DetailsCategory& InCategory, Node3D* InNode, DetailsTab& InTab) {
    WeakObjectPtr<Node3D> weak(InNode);
    UINode& body = *InCategory.GetBody();

    AddVectorRow(body, InTab, "Position", weak, "m_LocalPosition",
        [weak] { Node3D* node = weak.Get(); return node ? node->GetLocalPosition() : Vec3(0.0f); },
        [weak](const Vec3& InValue) { if (Node3D* node = weak.Get()) node->SetLocalPosition(InValue); },
        0.05, 0.0f);
    auto euler = std::make_shared<EulerEditState>();
    AddVectorRow(body, InTab, "Rotation", weak, "m_LocalRotation",
        [weak, euler] {
            Node3D* node = weak.Get();
            if (!node) {
                return Vec3(0.0f);
            }
            const Quat rotation = node->GetLocalRotation();
            if (!euler->Valid || !IsSameRotation(rotation, euler->Applied)) {
                euler->Angles = node->GetLocalEulerRotation();
                euler->Applied = rotation;
                euler->Valid = true;
            }
            return euler->Angles;
        },
        [weak, euler](const Vec3& InValue) {
            Node3D* node = weak.Get();
            if (!node) {
                return;
            }
            node->SetLocalEulerRotation(InValue);
            euler->Angles = InValue;
            euler->Applied = node->GetLocalRotation();
            euler->Valid = true;
        },
        0.5, 0.0f);
    AddVectorRow(body, InTab, "Scale", weak, "m_LocalScale",
        [weak] { Node3D* node = weak.Get(); return node ? node->GetLocalScale() : Vec3(1.0f); },
        [weak](const Vec3& InValue) { if (Node3D* node = weak.Get()) node->SetLocalScale(InValue); },
        0.05, 1.0f);
}
