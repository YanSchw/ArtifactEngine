#pragma once
#include "Graph/NodeGraph.h"
#include "Object/Enum.h"
#include "Object/Struct.h"
#include "AnimationStateMachine.gen.h"

class Animation;

ARTIFACT_ENUM();
enum class AnimationConditionMode : uint8_t {
    If = 0,
    IfNot,
    Greater,
    Less,
    Equals,
    NotEqual
};

ARTIFACT_ENUM();
enum class AnimationInterruptionSource : uint8_t {
    None = 0,
    CurrentState,
    NextState,
    CurrentStateThenNextState,
    NextStateThenCurrentState
};

struct AnimationCondition {
    ARTIFACT_STRUCT();

    PROPERTY()
    String Parameter;

    PROPERTY()
    AnimationConditionMode Mode = AnimationConditionMode::If;

    PROPERTY()
    float Threshold = 0.0f;

    bool IsMet(float InValue) const;
};

class AnimationTransition : public GraphConnection {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    bool HasExitTime = true;

    PROPERTY(Range = (0, 10))
    float ExitTime = 1.0f;

    PROPERTY()
    bool FixedDuration = true;

    PROPERTY(Range = (0, 10))
    float Duration = 0.1f;

    PROPERTY(Range = (0, 1))
    float Offset = 0.0f;

    PROPERTY()
    AnimationInterruptionSource InterruptionSource = AnimationInterruptionSource::None;

    PROPERTY()
    bool OrderedInterruption = true;

    PROPERTY()
    Array<AnimationCondition> Conditions;

    float GetDurationSeconds(float InSourceLength) const { return FixedDuration ? Duration : Duration * InSourceLength; }
};

class AnimationGraphNode : public GraphNode {
public:
    ARTIFACT_CLASS();

    inline static const String InPin = "In";
    inline static const String OutPin = "Out";
    inline static const String TransitionType = "Transition";

    virtual String GetCategory() const override { return "States"; }
};

class AnimationEntryNode : public AnimationGraphNode {
public:
    ARTIFACT_CLASS();

    inline static const String DefaultPin = "Default";

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Entry"; }
    virtual Vec4 GetAccentColor() const override;
    virtual bool IsUserCreatable() const override { return false; }
    virtual bool IsUserDeletable() const override { return false; }
};

class AnimationAnyStateNode : public AnimationGraphNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Any State"; }
    virtual Vec4 GetAccentColor() const override;
};

class AnimationExitNode : public AnimationGraphNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Exit"; }
    virtual Vec4 GetAccentColor() const override;
};

class AnimationStateNode : public AnimationGraphNode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    String StateName = "New State";

    PROPERTY()
    WeakObjectPtr<Animation> Motion;

    PROPERTY(Range = (-10, 10))
    float Speed = 1.0f;

    PROPERTY()
    Array<String> Tags;

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return StateName; }
    virtual Vec4 GetAccentColor() const override;
};

class AnimationStateMachine : public NodeGraph {
public:
    ARTIFACT_CLASS();

    AnimationStateMachine();

    AnimationEntryNode* GetEntry() const;
    AnimationEntryNode* GetOrCreateEntry();
    AnimationStateNode* GetDefaultState() const;
    void SetDefaultState(AnimationStateNode& InState);
    AnimationStateNode* FindState(const String& InName) const;
    AnimationStateNode* AddState(Animation* InMotion, const Vec2& InPosition);
    String MakeUniqueStateName(const String& InBase) const;

    Array<AnimationTransition*> GetTransitionsFrom(uint64_t InNodeId) const;
    Array<AnimationTransition*> GetAnyStateTransitions() const;

    virtual bool CanConnect(const GraphNode& InNodeA, const GraphPin& InPinA,
                            const GraphNode& InNodeB, const GraphPin& InPinB, String& OutReason) const override;
    virtual bool AllowsNodeClass(const Class& InNodeClass) const override;
    virtual bool AllowsMultipleConnections(const GraphPin& InPin) const override;
    virtual GraphConnection* MakeConnection(const GraphNode& InFrom, const GraphNode& InTo) const override;
    virtual void CollectNodeTemplates(Array<GraphNodeTemplate>& OutTemplates, Object* InContext) const override;
};
