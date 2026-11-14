#pragma once
#include "Node.h"
#include "NodeReference.h"
#include "Animation/AnimationPose.h"
#include "Assets/Asset.h"
#include "Common/Delegate.h"
#include "Common/Map.h"
#include "Object/Pointer.h"
#include "AnimatorNode.gen.h"

class Animation;
class AnimationGraph;
class AnimationStateNode;
class AnimationTransition;

ARTIFACT_ENUM();
enum class AnimatorSource : uint8_t {
    Animation = 0,
    AnimationGraph
};

struct AnimatorPlayback {
    WeakObjectPtr<Animation> Clip;
    uint64_t State = 0;
    float Speed = 1.0f;
    float Time = 0.0f;
    bool Started = false;
};

class AnimatorNode : public Node {
public:
    ARTIFACT_CLASS();

    virtual void BeginPlay() override;
    virtual void EndPlay() override;
    virtual void WorldUpdate(float InDeltaTime) override;

    PROPERTY(Params = (EventName))
    Delegate<const String&> OnAnimationEvent;

    FUNCTION()
    void Play();
    FUNCTION()
    void Stop();
    FUNCTION(Pure)
    bool IsPlaying() const { return m_Playing; }
    FUNCTION()
    void CrossFade(const String& InStateName, float InDuration = 0.1f);
    FUNCTION(Pure)
    String GetCurrentState() const;
    FUNCTION(Pure)
    float GetNormalizedTime() const;

    FUNCTION()
    void SetFloat(const String& InName, float InValue);
    FUNCTION(Pure)
    float GetFloat(const String& InName) const;
    FUNCTION()
    void SetInt(const String& InName, int32_t InValue);
    FUNCTION(Pure)
    int32_t GetInt(const String& InName) const;
    FUNCTION()
    void SetBool(const String& InName, bool InValue);
    FUNCTION(Pure)
    bool GetBool(const String& InName) const;
    FUNCTION()
    void SetTrigger(const String& InName);
    FUNCTION()
    void ResetTrigger(const String& InName);

    FUNCTION(Pure)
    Node* GetTarget();
    FUNCTION()
    void SetTarget(Node* InTarget);
    FUNCTION(Pure)
    AnimatorSource GetSource() const { return m_Source; }
    FUNCTION(Pure)
    Animation* GetAnimation() const;
    FUNCTION()
    void SetAnimation(Animation* InAnimation);
    FUNCTION(Pure)
    AnimationGraph* GetAnimationGraph() const;
    FUNCTION()
    void SetAnimationGraph(AnimationGraph* InGraph);
    FUNCTION(Pure)
    float GetSpeed() const { return m_Speed; }
    FUNCTION()
    void SetSpeed(float InSpeed) { m_Speed = InSpeed; }

    uint64_t GetCurrentStateId() const { return m_Current.State; }
    uint64_t GetNextStateId() const { return IsBlending() ? m_Next.State : 0; }
    void CollectEventNames(Array<String>& OutNames) const;

private:
    bool IsBlending() const { return m_BlendDuration > 0.0f; }
    void ResetParameters();
    void SetParameter(const String& InName, float InValue);
    float GetParameter(const String& InName) const;
    void Advance(AnimatorPlayback& OutPlayback, float InDeltaTime, bool InFireEvents);
    void SamplePlayback(const AnimatorPlayback& InPlayback, Node& InTarget, AnimationPose& OutPose) const;
    void ApplyPose(Node& InTarget);
    AnimatorPlayback MakePlayback(const AnimationStateNode* InState, float InOffset = 0.0f) const;
    void EnterState(const AnimationStateNode& InState, float InDuration, float InOffset, AnimationTransition* InTransition);
    void FinishBlend();
    AnimationTransition* FindTransition() const;
    AnimationTransition* FindTransitionFrom(const AnimatorPlayback& InPlayback, AnimationTransition* InActive) const;
    bool CanTake(const AnimationTransition& InTransition, const AnimatorPlayback& InPlayback) const;
    void Take(AnimationTransition& InTransition);

    PROPERTY()
    NodeReference m_Target = NodeReference(NodeReference::Parent);

    PROPERTY()
    AnimatorSource m_Source = AnimatorSource::Animation;

    PROPERTY()
    WeakObjectPtr<Animation> m_Animation;

    PROPERTY()
    WeakObjectPtr<AnimationGraph> m_AnimationGraph;

    PROPERTY()
    bool m_AutoPlay = true;

    PROPERTY(Range = (0, 10))
    float m_Speed = 1.0f;

    AnimatorPlayback m_Current;
    AnimatorPlayback m_Next;
    float m_BlendElapsed = 0.0f;
    float m_BlendDuration = 0.0f;
    WeakObjectPtr<AnimationTransition> m_ActiveTransition;
    AnimationPose m_LastPose;
    AnimationPose m_FrozenPose;
    bool m_HasFrozenPose = false;
    Map<String, float> m_Parameters;
    Array<AssetStreamHandle> m_Handles;
    bool m_Playing = false;
    bool m_WarnedTarget = false;
};
