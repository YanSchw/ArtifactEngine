#include "NodeDetailsCustomization.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "UI/UIDragNumber.h"
#include "UI/UICheckbox.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIHStack.h"
#include "Assets/AssetManager.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"
#include <cmath>
#include <memory>

float NodeDetailsCustomization::BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) {
    (void)InTab;
    Node* node = Cast<Node>(InObject);
    WeakObjectPtr<Node> weak(node);

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
    enabled->Changed = [weak](bool InValue) {
        if (Node* bound = weak.Get()) {
            bound->SetEnabled(InValue);
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
    const auto commitName = [weak](const String& InName) {
        if (Node* bound = weak.Get()) {
            if (!InName.empty()) {
                bound->SetName(InName);
            }
        }
    };
    name->Submitted = commitName;
    name->FocusLost = [name, commitName] { commitName(name->Text); };

    return 44.0f;
}

bool NodeDetailsCustomization::WantsClassCategory(const Class& InClass) const {
    return InClass == Node3D::StaticClass() || DetailsCustomization::WantsClassCategory(InClass);
}

void NodeDetailsCustomization::BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) {
    if (InClass == Node3D::StaticClass()) {
        if (Node3D* node = Cast<Node3D>(InObject)) {
            BuildTransformCategory(InCategory, node, InTab);
        }
        return;
    }
    DetailsCustomization::BuildClassCategory(InCategory, InClass, InObject, InTab);
}

static void AddVectorRow(UINode& InBody, DetailsTab& InTab, const String& InLabel,
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
        drag->Set = [InGet, InSet, axis](double InValue) {
            Vec3 value = InGet();
            value[axis] = (float)InValue;
            InSet(value);
        };
        drag->PrefixClicked = [InGet, InSet, axis, InAxisDefault] {
            Vec3 value = InGet();
            value[axis] = InAxisDefault;
            InSet(value);
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

    AddVectorRow(body, InTab, "Position",
        [weak] { Node3D* node = weak.Get(); return node ? node->GetLocalPosition() : Vec3(0.0f); },
        [weak](const Vec3& InValue) { if (Node3D* node = weak.Get()) node->SetLocalPosition(InValue); },
        0.05, 0.0f);
    auto euler = std::make_shared<EulerEditState>();
    AddVectorRow(body, InTab, "Rotation",
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
    AddVectorRow(body, InTab, "Scale",
        [weak] { Node3D* node = weak.Get(); return node ? node->GetLocalScale() : Vec3(1.0f); },
        [weak](const Vec3& InValue) { if (Node3D* node = weak.Get()) node->SetLocalScale(InValue); },
        0.05, 1.0f);
}
