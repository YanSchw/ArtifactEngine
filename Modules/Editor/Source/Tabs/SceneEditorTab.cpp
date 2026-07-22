#include "SceneEditorTab.h"
#include "OutlinerTab.h"
#include "DetailsTab.h"
#include "ViewportTab.h"
#include "GraphEditorTab.h"
#include "UI/UIDockArea.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/StaticMeshNode.h"
#include "Core/Log.h"

static void PopulateExampleWorld(World& InWorld) {
    // A single scene root keeps every node parented, so top-level nodes reorder by sibling index
    // just like nested ones (Unity's "SampleScene" root).
    Node* scene = InWorld.Spawn(Node3D::StaticClass());
    scene->SetName("Scene");

    scene->CreateChild(Node3D::StaticClass())->SetName("Directional Light");
    scene->CreateChild(CameraNode::StaticClass())->SetName("Main Camera");

    Node* environment = scene->CreateChild(Node3D::StaticClass());
    environment->SetName("Environment");
    environment->CreateChild(StaticMeshNode::StaticClass())->SetName("Floor");
    Node* props = environment->CreateChild(Node3D::StaticClass());
    props->SetName("Props");
    Node* crate = props->CreateChild(StaticMeshNode::StaticClass());
    crate->SetName("Crate");
    crate->GetTransform()->SetPosition(Vec3(-2.0f, 0.0f, 0.0f));
    Node* barrel = props->CreateChild(StaticMeshNode::StaticClass());
    barrel->SetName("Barrel");
    barrel->GetTransform()->SetPosition(Vec3(2.0f, 0.0f, 0.0f));

    Node* rig = scene->CreateChild(Node3D::StaticClass());
    rig->SetName("Character");
    Node* pelvis = rig->CreateChild(Node3D::StaticClass());
    pelvis->SetName("Pelvis");
    pelvis->CreateChild(Node3D::StaticClass())->SetName("Spine");
}

SceneEditorTab::SceneEditorTab() {
    World* world = new World();
    PopulateExampleWorld(*world);
    SetEditedWorld(world);

    UIDockArea* area = GetDockArea();
    area->DockNew<ViewportTab>(UIDockSlot::Center);
    OutlinerTab* outliner = area->DockNew<OutlinerTab>(UIDockSlot::Left, nullptr, 0.24f);
    area->DockNew<DetailsTab>(UIDockSlot::Bottom, outliner->GetDockNode(), 0.45f);
    area->DockNew<GraphEditorTab>(UIDockSlot::Bottom, nullptr, 0.42f);
}

VectorImage* SceneEditorTab::GetTabIcon() const {
    return EditorIcons::Level();
}

void SceneEditorTab::BuildToolBar(UINode& InToolBar) {
    const auto addButton = [&InToolBar](const String& InCaption, std::function<void()> InAction) {
        UIButton& button = UI::Button(InToolBar, InCaption, std::move(InAction));
        button.Size = { 70.0_px, 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
    };
    addButton("Save", [] { AE_INFO("SceneEditorTab: Save"); });
    addButton("Settings", [] { AE_INFO("SceneEditorTab: Settings"); });
    addButton("Play", [] { AE_INFO("SceneEditorTab: Play"); });
}
