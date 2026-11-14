#include "AnimationStateMachine.h"
#include "Assets/Animation.h"

static Vec4 Accent(uint32_t InHex) {
    return Vec4((float)((InHex >> 16) & 0xFF), (float)((InHex >> 8) & 0xFF), (float)(InHex & 0xFF), 255.0f) / 255.0f;
}

bool AnimationCondition::IsMet(float InValue) const {
    switch (Mode) {
        case AnimationConditionMode::If:       return InValue != 0.0f;
        case AnimationConditionMode::IfNot:    return InValue == 0.0f;
        case AnimationConditionMode::Greater:  return InValue > Threshold;
        case AnimationConditionMode::Less:     return InValue < Threshold;
        case AnimationConditionMode::Equals:   return InValue == Threshold;
        case AnimationConditionMode::NotEqual: return InValue != Threshold;
    }
    return false;
}

void AnimationEntryNode::ConstructPins() {
    AddOutput(DefaultPin, TransitionType);
}

Vec4 AnimationEntryNode::GetAccentColor() const {
    return Accent(0x1E7A3A);
}

void AnimationAnyStateNode::ConstructPins() {
    AddOutput(OutPin, TransitionType);
}

Vec4 AnimationAnyStateNode::GetAccentColor() const {
    return Accent(0x2F8F86);
}

void AnimationExitNode::ConstructPins() {
    AddInput(InPin, TransitionType);
}

Vec4 AnimationExitNode::GetAccentColor() const {
    return Accent(0xA8202A);
}

void AnimationStateNode::ConstructPins() {
    AddInput(InPin, TransitionType);
    AddOutput(OutPin, TransitionType);
}

Vec4 AnimationStateNode::GetAccentColor() const {
    return Accent(0x4A4D52);
}

AnimationStateMachine::AnimationStateMachine() {
    GraphName = "Animation Graph";
}

AnimationEntryNode* AnimationStateMachine::GetEntry() const {
    for (const SharedObjectPtr<GraphNode>& node : Nodes) {
        if (AnimationEntryNode* entry = Cast<AnimationEntryNode>(node.Get())) {
            return entry;
        }
    }
    return nullptr;
}

AnimationEntryNode* AnimationStateMachine::GetOrCreateEntry() {
    AnimationEntryNode* entry = GetEntry();
    return entry ? entry : CreateNode<AnimationEntryNode>(Vec2(-320.0f, 0.0f));
}

AnimationStateNode* AnimationStateMachine::GetDefaultState() const {
    AnimationEntryNode* entry = GetEntry();
    Array<AnimationTransition*> transitions = entry ? GetTransitionsFrom(entry->NodeId) : Array<AnimationTransition*>();
    return transitions.IsEmpty() ? nullptr : Cast<AnimationStateNode>(FindNode(transitions[0]->ToNodeId));
}

void AnimationStateMachine::SetDefaultState(AnimationStateNode& InState) {
    AnimationEntryNode* entry = GetOrCreateEntry();
    GraphPin* from = entry->FindPin(AnimationEntryNode::DefaultPin, GraphPinDirection::Output);
    GraphPin* to = InState.FindPin(AnimationGraphNode::InPin, GraphPinDirection::Input);
    if (from && to) {
        Connect(*entry, *from, InState, *to);
    }
}

AnimationStateNode* AnimationStateMachine::FindState(const String& InName) const {
    for (const SharedObjectPtr<GraphNode>& node : Nodes) {
        AnimationStateNode* state = Cast<AnimationStateNode>(node.Get());
        if (state && state->StateName == InName) {
            return state;
        }
    }
    return nullptr;
}

String AnimationStateMachine::MakeUniqueStateName(const String& InBase) const {
    const String base = InBase.empty() ? String("New State") : InBase;
    String name = base;
    for (int32_t suffix = 1; FindState(name); suffix++) {
        name = base + " " + std::to_string(suffix);
    }
    return name;
}

AnimationStateNode* AnimationStateMachine::AddState(Animation* InMotion, const Vec2& InPosition) {
    const String name = MakeUniqueStateName(InMotion ? InMotion->GetDisplayName() : String());
    AnimationStateNode* state = Cast<AnimationStateNode>(CreateNode(AnimationStateNode::StaticClass(), InPosition, [&](GraphNode& OutNode) {
        AnimationStateNode* created = Cast<AnimationStateNode>(&OutNode);
        created->StateName = name;
        created->Motion = InMotion;
    }));
    if (state && !GetDefaultState()) {
        SetDefaultState(*state);
    }
    return state;
}

Array<AnimationTransition*> AnimationStateMachine::GetTransitionsFrom(uint64_t InNodeId) const {
    Array<AnimationTransition*> transitions;
    for (const SharedObjectPtr<GraphConnection>& connection : Connections) {
        AnimationTransition* transition = Cast<AnimationTransition>(connection.Get());
        if (transition && transition->FromNodeId == InNodeId) {
            transitions.Add(transition);
        }
    }
    return transitions;
}

Array<AnimationTransition*> AnimationStateMachine::GetAnyStateTransitions() const {
    Array<AnimationTransition*> transitions;
    for (const SharedObjectPtr<GraphNode>& node : Nodes) {
        if (Cast<AnimationAnyStateNode>(node.Get())) {
            transitions += GetTransitionsFrom(node->NodeId);
        }
    }
    return transitions;
}

bool AnimationStateMachine::CanConnect(const GraphNode& InNodeA, const GraphPin& InPinA,
                                       const GraphNode& InNodeB, const GraphPin& InPinB, String& OutReason) const {
    if (!NodeGraph::CanConnect(InNodeA, InPinA, InNodeB, InPinB, OutReason)) {
        return false;
    }
    const GraphNode& from = InPinA.IsOutput() ? InNodeA : InNodeB;
    const GraphNode& to = InPinA.IsOutput() ? InNodeB : InNodeA;
    if ((Cast<AnimationEntryNode>(&from) || Cast<AnimationAnyStateNode>(&from)) && !Cast<AnimationStateNode>(&to)) {
        OutReason = from.GetTitle() + " can only lead to a state";
        return false;
    }
    return true;
}

bool AnimationStateMachine::AllowsNodeClass(const Class& InNodeClass) const {
    return InNodeClass.IsSubclassOf(AnimationGraphNode::StaticClass());
}

bool AnimationStateMachine::AllowsMultipleConnections(const GraphPin& InPin) const {
    return InPin.Name != AnimationEntryNode::DefaultPin;
}

GraphConnection* AnimationStateMachine::MakeConnection(const GraphNode& InFrom, const GraphNode& InTo) const {
    (void)InTo;
    AnimationTransition* transition = Object::Create<AnimationTransition>();
    const AnimationStateNode* state = Cast<AnimationStateNode>(&InFrom);
    Animation* motion = state ? state->Motion.Get() : nullptr;
    transition->HasExitTime = state && !(motion && motion->IsLooping());
    if (Cast<AnimationEntryNode>(&InFrom)) {
        transition->Duration = 0.0f;
    }
    return transition;
}

void AnimationStateMachine::CollectNodeTemplates(Array<GraphNodeTemplate>& OutTemplates, Object* InContext) const {
    (void)InContext;
    const String stateName = MakeUniqueStateName(String());
    OutTemplates.Add({ "States", "Empty State", String(), AnimationStateNode::StaticClass(), [stateName](GraphNode& OutNode) {
        Cast<AnimationStateNode>(&OutNode)->StateName = stateName;
    } });
    OutTemplates.Add({ "States", "Any State", String(), AnimationAnyStateNode::StaticClass(), nullptr });
    OutTemplates.Add({ "States", "Exit", String(), AnimationExitNode::StaticClass(), nullptr });
}
