#include "Blueprint.h"
#include "NodeRecord.h"
#include "AssetManager.h"
#include "GameFramework/Node.h"
#include "Core/Log.h"

static Array<UUID>& GetInstantiationStack() {
    static thread_local Array<UUID> stack;
    return stack;
}

static void AdoptAsBlueprintContent(Node& OutNode, bool InIsRoot) {
    OutNode.ClearAllPropertyOverrides();
    if (!InIsRoot) {
        OutNode.SetMarkedAsInherited(true);
    }
    for (uint32_t i = 0; i < OutNode.GetChildCount(); i++) {
        AdoptAsBlueprintContent(*OutNode.GetChild((int)i), false);
    }
}

Node* Blueprint::CreateInstance() const {
    if (!GetRoot()) {
        AE_WARN("Blueprint '{0}' has no node data", GetDisplayName());
        return nullptr;
    }

    Array<UUID>& stack = GetInstantiationStack();
    if (stack.Contains(GetId())) {
        AE_ERROR("Blueprint '{0}' contains itself; the recursive instance is skipped", GetDisplayName());
        return nullptr;
    }

    stack.Add(GetId());
    Node* node = GetRoot()->Instantiate(nullptr);
    stack.RemoveAt(stack.Last());

    if (!node) {
        return nullptr;
    }

    AdoptAsBlueprintContent(*node, true);
    node->SetName(GetDisplayName());
    node->m_BlueprintId = GetId();
    return node;
}

static bool RecordDependsOn(const NodeRecord& InRecord, const UUID& InBlueprintId, Array<UUID>& OutVisited) {
    const Class recordClass(InRecord.ClassName);
    if (recordClass.IsBlueprint()) {
        const UUID id = recordClass.GetBlueprintId();
        if (id == InBlueprintId) {
            return true;
        }
        if (!OutVisited.Contains(id)) {
            OutVisited.Add(id);
            Blueprint* nested = AssetManager::Get().GetAsset<Blueprint>(id);
            if (nested && nested->GetRoot() && RecordDependsOn(*nested->GetRoot(), InBlueprintId, OutVisited)) {
                return true;
            }
        }
    }

    for (const SharedObjectPtr<NodeRecord>& child : InRecord.Children) {
        if (child.Get() && RecordDependsOn(*child, InBlueprintId, OutVisited)) {
            return true;
        }
    }
    return false;
}

bool Blueprint::DependsOn(const UUID& InBlueprintId) const {
    if (!GetRoot()) {
        return false;
    }
    Array<UUID> visited;
    visited.Add(GetId());
    return RecordDependsOn(*GetRoot(), InBlueprintId, visited);
}

bool Blueprint::WouldRecurse(const UUID& InOwnerId, const UUID& InCandidateId) {
    if (!InOwnerId.IsValid() || !InCandidateId.IsValid()) {
        return false;
    }
    if (InOwnerId == InCandidateId) {
        return true;
    }
    Blueprint* candidate = AssetManager::Get().GetAsset<Blueprint>(InCandidateId);
    return candidate && candidate->DependsOn(InOwnerId);
}

Blueprint* Blueprint::CreateFromNode(Node& InNode, const String& InDirectory, const String& InName) {
    Blueprint* blueprint = Cast<Blueprint>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!blueprint) {
        return nullptr;
    }

    blueprint->CaptureFrom(InNode);
    if (NodeRecord* root = blueprint->GetRoot()) {
        root->Inherited = false;
    }
    AssetManager::Get().SaveAsset(blueprint);
    return blueprint;
}
