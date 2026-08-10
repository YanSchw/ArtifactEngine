#include "EditorTransaction.h"
#include "Tabs/MajorTab.h"
#include "Assets/NodeRecord.h"
#include "GameFramework/Node.h"
#include "GameFramework/World.h"
#include "Serialization/Json.h"

EditorSnapshot EditorTransaction::Capture(Object* InObject) {
    EditorSnapshot snapshot;
    snapshot.Target = InObject;
    snapshot.Values = JsonSerializer::SerializeObject(InObject);
    if (Node* node = Cast<Node>(InObject)) {
        snapshot.Children = NodeRecord::Capture(*node);
    }
    return snapshot;
}

void EditorTransaction::Apply(const EditorSnapshot& InSnapshot) {
    Object* target = InSnapshot.Target.Get();
    if (!target) {
        return;
    }
    JsonSerializer::DeserializeObject(target, InSnapshot.Values);

    Node* node = Cast<Node>(target);
    NodeRecord* record = InSnapshot.Children.Get();
    World* world = node ? node->GetWorld() : nullptr;
    if (!record || !world) {
        return;
    }

    node->SetName(record->Name);
    node->ClearAllPropertyOverrides();
    for (const auto& [name, value] : record->Values.items()) {
        node->MarkPropertyOverridden(name);
    }
    if (NodeRecord::Capture(*node)->ToJson() == record->ToJson()) {
        return;
    }

    while (node->HasChildren()) {
        node->GetChild(0)->Destroy();
    }
    world->ResolvePendingKills();
    record->Apply(*node);
}

void EditorTransaction::Record(Object* InObject) {
    for (const EditorSnapshot& snapshot : m_Snapshots) {
        if (snapshot.Target.Get() == InObject) {
            return;
        }
    }
    m_Snapshots.Add(Capture(InObject));
}

bool EditorTransaction::IsUnchanged() const {
    for (const EditorSnapshot& snapshot : m_Snapshots) {
        Object* target = snapshot.Target.Get();
        if (!target || Capture(target).Values != snapshot.Values) {
            return false;
        }
        if (snapshot.Children.Get() && NodeRecord::Capture(*Cast<Node>(target))->ToJson() != snapshot.Children->ToJson()) {
            return false;
        }
    }
    return true;
}

void EditorTransaction::Restore(MajorTab& InTab) {
    for (EditorSnapshot& snapshot : m_Snapshots) {
        Object* target = snapshot.Target.Get();
        if (!target) {
            continue;
        }
        const EditorSnapshot live = Capture(target);
        Apply(snapshot);
        snapshot = live;
        InTab.OnObjectEdited(target);
    }
}
