#include "AnimatorNode.h"
#include "Animation/AnimationPose.h"
#include "Animation/AnimationStateMachine.h"
#include "Assets/Animation.h"
#include "Assets/AnimationGraph.h"
#include "Core/Log.h"
#include <algorithm>
#include <cmath>

static float PlaybackLength(const AnimatorPlayback& InPlayback) {
    Animation* clip = InPlayback.Clip.Get();
    const float speed = std::abs(InPlayback.Speed);
    return clip && speed > 0.0f ? clip->GetDuration() / speed : 0.0f;
}

static float NormalizedTime(const AnimatorPlayback& InPlayback) {
    Animation* clip = InPlayback.Clip.Get();
    const float duration = clip ? clip->GetDuration() : 0.0f;
    return duration > 0.0f ? InPlayback.Time / duration : 0.0f;
}

void AnimatorNode::BeginPlay() {
    Super::BeginPlay();
    SetUpdateFlag(UpdateFlag::WorldUpdate);
    ResetParameters();
    if (m_AutoPlay) {
        Play();
    }
}

void AnimatorNode::EndPlay() {
    Stop();
    m_Handles.Clear();
    Super::EndPlay();
}

void AnimatorNode::WorldUpdate(float InDeltaTime) {
    Super::WorldUpdate(InDeltaTime);

    Node* target = GetTarget();
    if (!m_Playing || !target) {
        if (m_Playing && !m_WarnedTarget) {
            m_WarnedTarget = true;
            AE_WARN("Animator '{0}' has no target at '{1}'", GetName(), m_Target.Path);
        }
        return;
    }

    const float delta = InDeltaTime * m_Speed;
    Advance(m_Current, delta, !m_HasFrozenPose);
    if (IsBlending()) {
        Advance(m_Next, delta, true);
        m_BlendElapsed += delta;
        if (m_BlendElapsed >= m_BlendDuration) {
            FinishBlend();
        }
    }
    if (AnimationTransition* transition = FindTransition()) {
        Take(*transition);
    }

    if (m_Playing) {
        ApplyPose(*target);
    }
}

void AnimatorNode::Play() {
    m_Handles.Clear();
    FinishBlend();
    m_LastPose.Clear();
    m_WarnedTarget = false;

    if (AnimationGraph* graph = GetAnimationGraph()) {
        m_Handles.Add(graph->GetStreamHandle());
        AnimationStateNode* state = graph->GetStateMachine()->GetDefaultState();
        if (!state) {
            AE_WARN("Animation graph '{0}' has no default state", graph->GetDisplayName());
        }
        m_Current = MakePlayback(state);
    } else if (Animation* animation = GetAnimation()) {
        m_Handles.Add(animation->GetStreamHandle());
        m_Current = AnimatorPlayback();
        m_Current.Clip = animation;
    } else {
        m_Current = AnimatorPlayback();
    }
    m_Playing = true;
}

void AnimatorNode::Stop() {
    m_Playing = false;
}

void AnimatorNode::CrossFade(const String& InStateName, float InDuration) {
    AnimationGraph* graph = GetAnimationGraph();
    AnimationStateNode* state = graph ? graph->GetStateMachine()->FindState(InStateName) : nullptr;
    if (!state) {
        AE_WARN("Animator '{0}' has no state '{1}'", GetName(), InStateName);
        return;
    }
    if (!m_Playing) {
        Play();
    }
    EnterState(*state, InDuration, 0.0f, nullptr);
}

AnimationStateNode* AnimatorNode::FindCurrentState() const {
    AnimationGraph* graph = GetAnimationGraph();
    return graph ? Cast<AnimationStateNode>(graph->GetStateMachine()->FindNode(m_Current.State)) : nullptr;
}

String AnimatorNode::GetCurrentState() const {
    const AnimationStateNode* state = FindCurrentState();
    return state ? state->StateName : String();
}

Array<String> AnimatorNode::GetCurrentStateTags() const {
    const AnimationStateNode* state = FindCurrentState();
    return state ? state->Tags : Array<String>();
}

bool AnimatorNode::HasStateTag(const String& InTag) const {
    const AnimationStateNode* state = FindCurrentState();
    return state && state->Tags.Contains(InTag);
}

float AnimatorNode::GetNormalizedTime() const {
    return NormalizedTime(m_Current);
}

void AnimatorNode::ResetParameters() {
    m_Parameters.Clear();
    if (AnimationGraph* graph = GetAnimationGraph()) {
        for (const AnimationParameter& parameter : graph->GetParameters()) {
            m_Parameters[parameter.Name] = parameter.DefaultValue;
        }
    }
}

void AnimatorNode::SetParameter(const String& InName, float InValue) {
    if (!m_Parameters.ContainsKey(InName)) {
        AE_WARN("Animator '{0}' has no parameter '{1}'", GetName(), InName);
    }
    m_Parameters[InName] = InValue;
}

float AnimatorNode::GetParameter(const String& InName) const {
    const float* value = m_Parameters.Find(InName);
    return value ? *value : 0.0f;
}

void AnimatorNode::SetFloat(const String& InName, float InValue) {
    SetParameter(InName, InValue);
}

float AnimatorNode::GetFloat(const String& InName) const {
    return GetParameter(InName);
}

void AnimatorNode::SetInt(const String& InName, int32_t InValue) {
    SetParameter(InName, (float)InValue);
}

int32_t AnimatorNode::GetInt(const String& InName) const {
    return (int32_t)GetParameter(InName);
}

void AnimatorNode::SetBool(const String& InName, bool InValue) {
    SetParameter(InName, InValue ? 1.0f : 0.0f);
}

bool AnimatorNode::GetBool(const String& InName) const {
    return GetParameter(InName) != 0.0f;
}

void AnimatorNode::SetTrigger(const String& InName) {
    SetParameter(InName, 1.0f);
}

void AnimatorNode::ResetTrigger(const String& InName) {
    SetParameter(InName, 0.0f);
}

Node* AnimatorNode::GetTarget() {
    return m_Target.Resolve(*this);
}

void AnimatorNode::SetTarget(Node* InTarget) {
    m_Target = InTarget ? NodeReference::Between(*this, *InTarget) : NodeReference();
    m_WarnedTarget = false;
}

Animation* AnimatorNode::GetAnimation() const {
    return m_Source == AnimatorSource::Animation ? m_Animation.Get() : nullptr;
}

void AnimatorNode::SetAnimation(Animation* InAnimation) {
    m_Source = AnimatorSource::Animation;
    m_Animation = InAnimation;
    if (m_Playing) {
        Play();
    }
}

AnimationGraph* AnimatorNode::GetAnimationGraph() const {
    return m_Source == AnimatorSource::AnimationGraph ? m_AnimationGraph.Get() : nullptr;
}

void AnimatorNode::SetAnimationGraph(AnimationGraph* InGraph) {
    m_Source = AnimatorSource::AnimationGraph;
    m_AnimationGraph = InGraph;
    m_Current = AnimatorPlayback();
    ResetParameters();
    if (m_Playing) {
        Play();
    }
}

void AnimatorNode::CollectEventNames(Array<String>& OutNames) const {
    if (AnimationGraph* graph = GetAnimationGraph()) {
        graph->CollectEventNames(OutNames);
    }
    if (Animation* animation = GetAnimation()) {
        for (const AnimationEvent& event : animation->GetEvents()) {
            if (!event.Name.empty() && !OutNames.Contains(event.Name)) {
                OutNames.Add(event.Name);
            }
        }
    }
}

void AnimatorNode::Advance(AnimatorPlayback& OutPlayback, float InDeltaTime, bool InFireEvents) {
    Animation* clip = OutPlayback.Clip.Get();
    const float from = OutPlayback.Started ? OutPlayback.Time : OutPlayback.Time - 0.5f / (clip ? clip->GetFrameRate() : 1.0f);
    OutPlayback.Time += InDeltaTime * OutPlayback.Speed;
    OutPlayback.Started = true;
    if (!clip || !InFireEvents) {
        return;
    }

    Array<String> passed;
    clip->CollectEventsBetween(from * clip->GetFrameRate(), OutPlayback.Time * clip->GetFrameRate(), passed);
    for (const String& name : passed) {
        OnAnimationEvent.Broadcast(name);
    }
}

void AnimatorNode::SamplePlayback(const AnimatorPlayback& InPlayback, Node& InTarget, AnimationPose& OutPose) const {
    if (Animation* clip = InPlayback.Clip.Get()) {
        clip->Sample(InTarget, clip->FrameAtTime(InPlayback.Time), OutPose);
    }
}

void AnimatorNode::ApplyPose(Node& InTarget) {
    AnimationPose pose;
    if (m_HasFrozenPose) {
        pose = m_FrozenPose;
    } else {
        SamplePlayback(m_Current, InTarget, pose);
    }
    if (IsBlending()) {
        AnimationPose next;
        SamplePlayback(m_Next, InTarget, next);
        pose.BlendTowards(next, std::clamp(m_BlendElapsed / m_BlendDuration, 0.0f, 1.0f));
    }
    pose.Apply();
    m_LastPose = std::move(pose);
}

AnimatorPlayback AnimatorNode::MakePlayback(const AnimationStateNode* InState, float InOffset) const {
    AnimatorPlayback playback;
    if (InState) {
        playback.Clip = InState->Motion;
        playback.State = InState->NodeId;
        playback.Speed = InState->Speed;
    }
    if (Animation* clip = playback.Clip.Get()) {
        playback.Time = InOffset * clip->GetDuration();
    }
    return playback;
}

void AnimatorNode::EnterState(const AnimationStateNode& InState, float InDuration, float InOffset, AnimationTransition* InTransition) {
    const AnimatorPlayback next = MakePlayback(&InState, InOffset);
    if (InDuration <= 0.0f) {
        FinishBlend();
        m_Current = next;
        return;
    }
    m_HasFrozenPose = IsBlending();
    if (m_HasFrozenPose) {
        m_FrozenPose = m_LastPose;
    }
    m_Next = next;
    m_BlendElapsed = 0.0f;
    m_BlendDuration = InDuration;
    m_ActiveTransition = InTransition;
}

void AnimatorNode::FinishBlend() {
    if (IsBlending()) {
        m_Current = m_Next;
    }
    m_Next = AnimatorPlayback();
    m_BlendDuration = 0.0f;
    m_ActiveTransition = nullptr;
    m_HasFrozenPose = false;
    m_FrozenPose.Clear();
}

AnimationTransition* AnimatorNode::FindTransition() const {
    AnimationGraph* graph = GetAnimationGraph();
    if (!graph || m_Current.State == 0) {
        return nullptr;
    }
    const AnimatorPlayback& latest = IsBlending() ? m_Next : m_Current;
    for (AnimationTransition* transition : graph->GetStateMachine()->GetAnyStateTransitions()) {
        if (transition->ToNodeId != latest.State && CanTake(*transition, latest)) {
            return transition;
        }
    }
    if (!IsBlending()) {
        return FindTransitionFrom(m_Current, nullptr);
    }

    AnimationTransition* active = m_ActiveTransition.Get();
    AnimationTransition* found = nullptr;
    switch (active ? active->InterruptionSource : AnimationInterruptionSource::None) {
        case AnimationInterruptionSource::None:
            break;
        case AnimationInterruptionSource::CurrentState:
            found = FindTransitionFrom(m_Current, active);
            break;
        case AnimationInterruptionSource::NextState:
            found = FindTransitionFrom(m_Next, active);
            break;
        case AnimationInterruptionSource::CurrentStateThenNextState:
            found = FindTransitionFrom(m_Current, active);
            found = found ? found : FindTransitionFrom(m_Next, active);
            break;
        case AnimationInterruptionSource::NextStateThenCurrentState:
            found = FindTransitionFrom(m_Next, active);
            found = found ? found : FindTransitionFrom(m_Current, active);
            break;
    }
    return found;
}

AnimationTransition* AnimatorNode::FindTransitionFrom(const AnimatorPlayback& InPlayback, AnimationTransition* InActive) const {
    for (AnimationTransition* transition : GetAnimationGraph()->GetStateMachine()->GetTransitionsFrom(InPlayback.State)) {
        if (transition == InActive && InActive->OrderedInterruption) {
            break;
        }
        const bool redundant = transition == InActive || (IsBlending() && transition->ToNodeId == m_Next.State);
        if (!redundant && CanTake(*transition, InPlayback)) {
            return transition;
        }
    }
    return nullptr;
}

bool AnimatorNode::CanTake(const AnimationTransition& InTransition, const AnimatorPlayback& InPlayback) const {
    if (InTransition.HasExitTime && NormalizedTime(InPlayback) < InTransition.ExitTime) {
        return false;
    }
    for (const AnimationCondition& condition : InTransition.Conditions) {
        const float* value = m_Parameters.Find(condition.Parameter);
        if (!value || !condition.IsMet(*value)) {
            return false;
        }
    }
    return true;
}

void AnimatorNode::Take(AnimationTransition& InTransition) {
    AnimationGraph* graph = GetAnimationGraph();
    AnimationStateMachine* machine = graph->GetStateMachine();
    for (const AnimationCondition& condition : InTransition.Conditions) {
        const AnimationParameter* parameter = graph->FindParameter(condition.Parameter);
        if (parameter && parameter->Type == AnimationParameterType::Trigger) {
            m_Parameters[condition.Parameter] = 0.0f;
        }
    }

    GraphNode* destination = machine->FindNode(InTransition.ToNodeId);
    if (Cast<AnimationExitNode>(destination)) {
        destination = machine->GetDefaultState();
    }
    AnimationStateNode* state = Cast<AnimationStateNode>(destination);
    if (!state) {
        return;
    }
    if (IsBlending() && InTransition.FromNodeId == m_Next.State) {
        m_Current = m_Next;
    }
    EnterState(*state, InTransition.GetDurationSeconds(PlaybackLength(m_Current)), InTransition.Offset, &InTransition);
}
