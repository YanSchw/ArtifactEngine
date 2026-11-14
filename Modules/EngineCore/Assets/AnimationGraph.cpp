#include "AnimationGraph.h"
#include "Animation.h"
#include "AssetManager.h"

AnimationGraph* AnimationGraph::CreateEmpty(const String& InDirectory, const String& InName) {
    AnimationGraph* graph = Cast<AnimationGraph>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!graph) {
        return nullptr;
    }

    AnimationStateMachine* machine = graph->GetStateMachine();
    machine->CreateNode<AnimationAnyStateNode>(Vec2(-352.0f, -112.0f));
    machine->CreateNode<AnimationExitNode>(Vec2(352.0f, 0.0f));

    AssetManager::Get().SaveAsset(graph);
    return graph;
}

AnimationStateMachine* AnimationGraph::GetStateMachine() {
    if (!m_StateMachine) {
        m_StateMachine = SharedObjectPtr<AnimationStateMachine>(Object::Create<AnimationStateMachine>());
        m_StateMachine->GetOrCreateEntry();
    }
    return m_StateMachine.Get();
}

const AnimationParameter* AnimationGraph::FindParameter(const String& InName) const {
    for (const AnimationParameter& parameter : m_Parameters) {
        if (parameter.Name == InName) {
            return &parameter;
        }
    }
    return nullptr;
}

String AnimationGraph::MakeUniqueParameterName(const String& InBase) const {
    String name = InBase;
    for (int32_t suffix = 1; FindParameter(name); suffix++) {
        name = InBase + " " + std::to_string(suffix);
    }
    return name;
}

int32_t AnimationGraph::AddParameter(AnimationParameterType InType) {
    AnimationParameter parameter;
    parameter.Type = InType;
    parameter.Name = MakeUniqueParameterName("New " + Enum("AnimationParameterType").ConvertValueToString((int64_t)InType));
    m_Parameters.Add(parameter);
    return m_Parameters.Last();
}

void AnimationGraph::RenameParameter(int32_t InIndex, const String& InName) {
    if (InIndex < 0 || InIndex >= m_Parameters.Size() || InName.empty() || FindParameter(InName)) {
        return;
    }
    const String previous = m_Parameters[InIndex].Name;
    m_Parameters[InIndex].Name = InName;
    for (const SharedObjectPtr<GraphConnection>& connection : GetStateMachine()->Connections) {
        if (AnimationTransition* transition = Cast<AnimationTransition>(connection.Get())) {
            for (AnimationCondition& condition : transition->Conditions) {
                if (condition.Parameter == previous) {
                    condition.Parameter = InName;
                }
            }
        }
    }
}

void AnimationGraph::SetParameterDefault(int32_t InIndex, float InValue) {
    if (InIndex >= 0 && InIndex < m_Parameters.Size()) {
        m_Parameters[InIndex].DefaultValue = InValue;
    }
}

void AnimationGraph::RemoveParameter(int32_t InIndex) {
    if (InIndex >= 0 && InIndex < m_Parameters.Size()) {
        m_Parameters.RemoveAt(InIndex);
    }
}

void AnimationGraph::CollectEventNames(Array<String>& OutNames) const {
    if (!m_StateMachine) {
        return;
    }
    for (const SharedObjectPtr<GraphNode>& node : m_StateMachine->Nodes) {
        AnimationStateNode* state = Cast<AnimationStateNode>(node.Get());
        Animation* motion = state ? state->Motion.Get() : nullptr;
        if (!motion) {
            continue;
        }
        for (const AnimationEvent& event : motion->GetEvents()) {
            if (!event.Name.empty() && !OutNames.Contains(event.Name)) {
                OutNames.Add(event.Name);
            }
        }
    }
}

void AnimationGraph::Load() {
    m_Loaded = true;
    m_Motions.Clear();
    AnimationStateMachine* machine = GetStateMachine();
    machine->GetOrCreateEntry();
    for (const SharedObjectPtr<GraphNode>& node : machine->Nodes) {
        AnimationStateNode* state = Cast<AnimationStateNode>(node.Get());
        if (Animation* motion = state ? state->Motion.Get() : nullptr) {
            m_Motions.Add(motion->GetStreamHandle());
        }
    }
}

void AnimationGraph::Unload() {
    m_Motions.Clear();
    m_Loaded = false;
}
