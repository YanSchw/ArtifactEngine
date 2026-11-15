#include "BlueprintEditorTab.h"
#include "EventGraphTab.h"
#include "UI/UIDockNode.h"
#include "OutlinerTab.h"
#include "DetailsTab.h"
#include "ViewportTab.h"
#include "UI/UIDockArea.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIDropdown.h"
#include "UI/UIClassPicker.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/Node.h"
#include "GameFramework/Component.h"
#include "Assets/AssetManager.h"
#include "Assets/Blueprint.h"
#include "Assets/NodeRecord.h"
#include "Core/Log.h"

BlueprintEditorTab::BlueprintEditorTab() {
    SetEditedWorld(new World());

    UIDockArea* area = GetDockArea();
    m_Viewport = area->DockNew<ViewportTab>(UIDockSlot::Center);
    area->DockNew<EventGraphTab>(UIDockSlot::Center, m_Viewport->GetDockNode());
    m_Viewport->GetDockNode()->SetActiveTab(m_Viewport);
    OutlinerTab* outliner = area->DockNew<OutlinerTab>(UIDockSlot::Left, nullptr, 0.24f);
    area->DockNew<DetailsTab>(UIDockSlot::Bottom, outliner->GetDockNode(), 0.45f);
}

void BlueprintEditorTab::DestroyInstance() {
    ClearSelection();
    if (Node* previous = m_Instance.Get()) {
        previous->Destroy();
        GetEditedWorld()->ResolvePendingKills();
    }
    m_Instance = nullptr;
}

void BlueprintEditorTab::BuildInstanceFrom(NodeRecord& InRecord) {
    DestroyInstance();

    Node* instance = GetEditedWorld()->Spawn(Class(InRecord.ClassName));
    if (!instance) {
        AE_ERROR("'{0}' cannot be instantiated as a Blueprint root", InRecord.ClassName);
        return;
    }

    if (!InRecord.Name.empty()) {
        instance->SetName(InRecord.Name);
    }
    InRecord.Apply(*instance);
    m_Instance = instance;

    if (Blueprint* blueprint = m_Blueprint.Get()) {
        instance->SetName(blueprint->GetDisplayName());
    }
    SyncViewportMode();

    SetSelection(instance);
}

void BlueprintEditorTab::SyncViewportMode() {
    Node* instance = m_Instance.Get();
    if (!m_Viewport || !instance) {
        return;
    }
    const int8_t isUI = instance->IsA<UINode>() ? 1 : 0;
    if (m_RootWasUI != isUI) {
        m_RootWasUI = isUI;
        m_Viewport->SetDesignMode(isUI != 0);
    }
}

void BlueprintEditorTab::OpenBlueprint(Blueprint* InBlueprint) {
    DestroyInstance();
    m_Blueprint = InBlueprint;

    if (!InBlueprint) {
        return;
    }

    AssetManager::Get().LoadAsset(InBlueprint);
    NodeRecord* record = InBlueprint->GetRoot();
    if (!record) {
        AE_WARN("Blueprint '{0}' has no node data", InBlueprint->GetDisplayName());
        return;
    }

    BuildInstanceFrom(*record);
}

Class BlueprintEditorTab::GetParentClass() const {
    if (Node* instance = m_Instance.Get()) {
        return instance->GetSerializedClass();
    }
    Blueprint* blueprint = m_Blueprint.Get();
    return blueprint ? blueprint->GetRootClass() : Class::None;
}

Array<Class> BlueprintEditorTab::GetSelectableParentBlueprints() const {
    Blueprint* blueprint = m_Blueprint.Get();
    Array<Class> classes;
    for (Asset* asset : AssetManager::Get().GetAssetsOfClass(Blueprint::StaticClass())) {
        Blueprint* candidate = Cast<Blueprint>(asset);
        if (!candidate) {
            continue;
        }
        if (blueprint && Blueprint::WouldRecurse(blueprint->GetId(), candidate->GetId())) {
            continue;
        }
        classes.Add(Class::FromBlueprint(candidate->GetId()));
    }
    return classes;
}

void BlueprintEditorTab::SetParentClass(const Class& InClass) {
    Blueprint* blueprint = m_Blueprint.Get();
    Node* instance = m_Instance.Get();
    if (!blueprint || !instance || InClass == GetParentClass()) {
        return;
    }
    if (InClass.IsBlueprint() && Blueprint::WouldRecurse(blueprint->GetId(), InClass.GetBlueprintId())) {
        AE_ERROR("'{0}' cannot be the parent of '{1}'; that would nest the Blueprint inside itself",
                 InClass.GetDisplayName(), blueprint->GetDisplayName());
        return;
    }

    SharedObjectPtr<NodeRecord> state = NodeRecord::Capture(*instance);
    state->ClassName = InClass.Name;
    state->Inherited = false;
    BuildInstanceFrom(*state);
}

void BlueprintEditorTab::Save() {
    Blueprint* blueprint = m_Blueprint.Get();
    Node* instance = m_Instance.Get();
    if (!blueprint || !instance) {
        AE_WARN("There is no blueprint open to save");
        return;
    }

    SharedObjectPtr<NodeRecord> previous = blueprint->GetRoot();
    blueprint->CaptureFrom(*instance);
    if (NodeRecord* root = blueprint->GetRoot()) {
        root->Inherited = false;
    }

    if (blueprint->DependsOn(blueprint->GetId())) {
        AE_ERROR("'{0}' would contain itself; the save was rejected", blueprint->GetDisplayName());
        blueprint->SetRoot(previous);
        return;
    }

    if (AssetManager::Get().SaveAsset(blueprint)) {
        BroadcastAssetSaved(blueprint, this);
    }
}

Asset* BlueprintEditorTab::GetEditedAsset() const {
    return m_Blueprint.Get();
}

Node* BlueprintEditorTab::GetAssetRootNode() const {
    return m_Instance.Get();
}

void BlueprintEditorTab::OnAssetSaved(Asset* InAsset) {
    Blueprint* blueprint = m_Blueprint.Get();
    if (!blueprint) {
        return;
    }

    Blueprint* saved = Cast<Blueprint>(InAsset);
    if (saved == blueprint || (saved && blueprint->DependsOn(saved->GetId()))) {
        OpenBlueprint(blueprint);
    }
}

String BlueprintEditorTab::GetTabTitle() const {
    Blueprint* blueprint = m_Blueprint.Get();
    return blueprint ? blueprint->GetDisplayName() : String("Untitled Blueprint");
}

VectorImage* BlueprintEditorTab::GetTabIcon() const {
    return EditorIcons::Asset();
}

void BlueprintEditorTab::BuildToolBar(UINode& InToolBar) {
    EditorStyle::ToolButton(InToolBar, EditorIcons::Save(), "Save", [this] { Save(); });

    UILabel* caption = InToolBar.Add<UILabel>();
    caption->Size = { 78.0_px, 1.0_rel };
    caption->FontSize = EditorStyle::FontSize;
    caption->Color = EditorStyle::TextDim;
    caption->HAlign = UIHAlign::Right;
    caption->VAlign = UIVAlign::Middle;
    caption->Text = "Parent Class";

    UIDropdown* parent = InToolBar.Add<UIDropdown>();
    parent->Size = { 200.0_px, 1.0_rel };
    parent->PopupSize = Vec2(300.0f, 380.0f);
    parent->GetSelectedLabel = [this] { return GetParentClass().GetDisplayName(); };
    parent->GetSelectedIcon = [this] { return EditorIcons::GetNodeIcon(GetParentClass()); };
    parent->BuildPopup = [this, parent](UINode& InPanel) {
        UIClassPicker* picker = InPanel.Add<UIClassPicker>();
        picker->Fill();
        picker->Selected = GetParentClass();
        picker->Filter = &UIClassPicker::IsPlaceableNodeClass;
        picker->ExtraClasses = GetSelectableParentBlueprints();
        picker->SelectionChanged = [this, parent](const Class& InClass) {
            SetParentClass(InClass);
            parent->ClosePopup();
        };
        picker->FocusSearch();
    };
}
