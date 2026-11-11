#include "EventGraphSubsystem.h"
#include "EventGraph.h"
#include "EventGraphInstance.h"
#include "Assets/AssetManager.h"
#include "Assets/Blueprint.h"
#include "Assets/Scene.h"
#include "GameFramework/SceneRootNode.h"

EventGraph* EventGraphSubsystem::FindGraphFor(Node& InNode) {
    NodeAsset* asset = nullptr;
    if (InNode.GetBlueprintId().IsValid() && AssetManager::IsAvailable()) {
        asset = AssetManager::Get().GetAsset<Blueprint>(InNode.GetBlueprintId());
    } else if (SceneRootNode* root = Cast<SceneRootNode>(&InNode)) {
        asset = root->GetScene();
    }
    EventGraph* graph = asset ? asset->GetEventGraph() : nullptr;
    return graph && !graph->Nodes.IsEmpty() ? graph : nullptr;
}

EventGraphInstance* EventGraphSubsystem::FindInstance(const Node& InSelf) const {
    for (const SharedObjectPtr<EventGraphInstance>& instance : m_Instances) {
        if (instance->IsRunning() && instance->GetSelf() == &InSelf) {
            return instance.Get();
        }
    }
    return nullptr;
}

void EventGraphSubsystem::OnNodeBeginPlay(Node& InNode) {
    EventGraph* graph = FindGraphFor(InNode);
    if (!graph || FindInstance(InNode)) {
        return;
    }
    EventGraphInstance* instance = Object::Create<EventGraphInstance>();
    m_Instances.Add(SharedObjectPtr<EventGraphInstance>(instance));
    instance->Start(*graph, InNode);
}

void EventGraphSubsystem::OnNodeEndPlay(Node& InNode) {
    if (EventGraphInstance* instance = FindInstance(InNode)) {
        instance->Stop();
    }
}

void EventGraphSubsystem::WorldUpdate(float InDeltaTime) {
    for (int32_t i = 0; i < m_Instances.Size(); i++) {
        SharedObjectPtr<EventGraphInstance> instance = m_Instances[i];
        if (instance->IsRunning() && !instance->HasBegunPlay()) {
            instance->BeginPlay();
        }
        if (instance->IsRunning()) {
            instance->Tick(InDeltaTime);
        }
    }
    m_Instances.Filter([](const SharedObjectPtr<EventGraphInstance>& InInstance) { return InInstance->IsRunning(); });
}

void EventGraphSubsystem::Shutdown() {
    for (const SharedObjectPtr<EventGraphInstance>& instance : m_Instances) {
        instance->Stop();
    }
    m_Instances.Clear();
}
