#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Delegate.h"
#include "Common/Map.h"
#include <memory>
#include "EventGraphInstance.gen.h"

class EventGraph;
class EventGraphNode;
class GraphNode;
class Node;

class EventGraphInstance : public Object {
public:
    ARTIFACT_CLASS();

    inline static constexpr int32_t MaxSteps = 10000;

    void Start(EventGraph& InGraph, Node& InSelf);
    void BeginPlay();
    void Tick(float InDeltaTime);
    void Stop();

    bool IsRunning() const { return m_Running; }
    bool HasBegunPlay() const { return m_BegunPlay; }
    Node* GetSelf() const { return m_Self.Get(); }
    Node* FindNode(const String& InName) const;
    bool IsPinConnected(const GraphNode& InNode, const String& InPinName) const;

    void* GetFrame(const EventGraphNode& InNode);
    void RunEvent(const EventGraphNode& InEvent);
    void BindDelegate(const EventGraphNode& InEvent, DelegateProperty& InDelegate, Object& InTarget);
    void ReportError(const GraphNode& InNode, const String& InMessage) const;

private:
    struct Binding {
        DelegateProperty* Delegate = nullptr;
        WeakObjectPtr<Object> Target;
        DelegateHandle Handle;
    };

    template<typename T>
    void RunEvents(const std::function<void(const T&, void*)>& InPrepare = nullptr);

    void RunFrom(const EventGraphNode& InNode, const String& InExecPin);
    const EventGraphNode* FollowExec(const EventGraphNode& InNode, const String& InExecPin) const;
    bool PullInputs(const EventGraphNode& InNode, void* InFrame);
    bool Evaluate(const EventGraphNode& InNode);

    SharedObjectPtr<EventGraph> m_Graph;
    WeakObjectPtr<Node> m_Self;
    Map<uint64_t, std::shared_ptr<void>> m_Frames;
    Array<Binding> m_Bindings;
    Array<uint64_t> m_Evaluating;
    int32_t m_Steps = 0;
    int32_t m_Depth = 0;
    bool m_Running = false;
    bool m_BegunPlay = false;
};
