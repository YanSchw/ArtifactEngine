#include "EventGraphInstance.h"
#include "EventGraph.h"
#include "EventGraphNodes.h"
#include "GameFramework/Node.h"
#include "Core/Log.h"

void EventGraphInstance::Start(EventGraph& InGraph, Node& InSelf) {
    m_Graph = &InGraph;
    m_Self = &InSelf;
    m_Running = true;
    InGraph.RefreshNodes();
    for (const SharedObjectPtr<GraphNode>& graphNode : InGraph.Nodes) {
        if (EventGraphNode* node = Cast<EventGraphNode>(graphNode.Get())) {
            node->Start(*this);
        }
    }
}

template<typename T>
void EventGraphInstance::RunEvents(const std::function<void(const T&, void*)>& InPrepare) {
    if (!m_Graph) {
        return;
    }
    Array<uint64_t> events;
    for (const SharedObjectPtr<GraphNode>& graphNode : m_Graph->Nodes) {
        if (Cast<T>(graphNode.Get())) {
            events.Add(graphNode->NodeId);
        }
    }
    for (uint64_t id : events) {
        const T* event = m_Running && m_Graph ? Cast<T>(m_Graph->FindNode(id)) : nullptr;
        if (!event) {
            continue;
        }
        if (InPrepare) {
            InPrepare(*event, GetFrame(*event));
        }
        RunEvent(*event);
    }
}

void EventGraphInstance::BeginPlay() {
    m_BegunPlay = true;
    RunEvents<EventBeginPlayNode>();
}

void EventGraphInstance::Tick(float InDeltaTime) {
    Node* self = m_Self.Get();
    if (!self || !self->IsEnabled()) {
        return;
    }
    RunEvents<EventTickNode>([InDeltaTime](const EventTickNode& InEvent, void* InFrame) {
        const EventGraphPinValue value = InEvent.GetPinValue(InFrame, "DeltaSeconds");
        if (value.Data) {
            *(float*)value.Data = InDeltaTime;
        }
    });
}

void EventGraphInstance::Stop() {
    if (!m_Running) {
        return;
    }
    if (m_BegunPlay) {
        RunEvents<EventEndPlayNode>();
    }
    m_Running = false;
    for (const Binding& binding : m_Bindings) {
        if (Object* target = binding.Target.Get()) {
            binding.Delegate->Unbind(binding.Delegate->GetValuePtr(target), binding.Handle);
        }
    }
    m_Bindings.Clear();
}

Node* EventGraphInstance::FindNode(const String& InName) const {
    Node* self = m_Self.Get();
    return self ? self->GetChildByName(InName, true) : nullptr;
}

bool EventGraphInstance::IsPinConnected(const GraphNode& InNode, const String& InPinName) const {
    GraphPin* pin = InNode.FindPin(InPinName, GraphPinDirection::Input);
    pin = pin ? pin : InNode.FindPin(InPinName, GraphPinDirection::Output);
    return m_Graph && pin && m_Graph->IsPinConnected(InNode, *pin);
}

void* EventGraphInstance::GetFrame(const EventGraphNode& InNode) {
    if (std::shared_ptr<void>* frame = m_Frames.Find(InNode.NodeId)) {
        return frame->get();
    }
    std::shared_ptr<void> frame = InNode.MakeFrameWithDefaults();
    m_Frames[InNode.NodeId] = frame;
    return frame.get();
}

void EventGraphInstance::BindDelegate(const EventGraphNode& InEvent, DelegateProperty& InDelegate, Object& InTarget) {
    const uint64_t eventId = InEvent.NodeId;
    const DelegateHandle handle = InDelegate.Bind(InDelegate.GetValuePtr(&InTarget), this, [this, eventId, &InDelegate](void* InParams) {
        const EventGraphNode* event = m_Running && m_Graph ? Cast<EventGraphNode>(m_Graph->FindNode(eventId)) : nullptr;
        if (!event) {
            return;
        }
        InDelegate.SignatureFunction->CopyParams(GetFrame(*event), InParams);
        RunEvent(*event);
    });
    m_Bindings.Add({ &InDelegate, &InTarget, handle });
}

void EventGraphInstance::ReportError(const GraphNode& InNode, const String& InMessage) const {
    Node* self = m_Self.Get();
    AE_WARN("Event graph of '{0}', node '{1}': {2}", self ? self->GetName() : String("?"), InNode.GetTitle(), InMessage);
}

void EventGraphInstance::RunEvent(const EventGraphNode& InEvent) {
    SharedObjectPtr<EventGraphInstance> keepAlive = this;
    if (m_Depth == 0) {
        m_Steps = 0;
    }
    m_Depth++;
    RunFrom(InEvent, EventGraphNode::ThenPin);
    m_Depth--;
}

void EventGraphInstance::RunFrom(const EventGraphNode& InNode, const String& InExecPin) {
    const EventGraphNode* node = FollowExec(InNode, InExecPin);
    while (node && m_Running) {
        if (++m_Steps > MaxSteps) {
            ReportError(*node, "stopped after " + std::to_string(MaxSteps) + " steps, is there an endless loop?");
            return;
        }
        void* frame = GetFrame(*node);
        if (!frame || !PullInputs(*node, frame)) {
            return;
        }
        const uint64_t nodeId = node->NodeId;
        const String next = node->Execute(*this, frame);
        const EventGraphNode* current = m_Graph ? Cast<EventGraphNode>(m_Graph->FindNode(nodeId)) : nullptr;
        node = next.empty() || !current ? nullptr : FollowExec(*current, next);
    }
}

const EventGraphNode* EventGraphInstance::FollowExec(const EventGraphNode& InNode, const String& InExecPin) const {
    for (const SharedObjectPtr<GraphConnection>& connection : m_Graph->Connections) {
        if (connection && connection->UsesOutput(InNode.NodeId, InExecPin)) {
            return Cast<EventGraphNode>(m_Graph->FindNode(connection->ToNodeId));
        }
    }
    return nullptr;
}

bool EventGraphInstance::PullInputs(const EventGraphNode& InNode, void* InFrame) {
    for (const SharedObjectPtr<GraphPin>& pin : InNode.Pins) {
        if (!pin->IsInput() || pin->TypeName == EventGraph::ExecType) {
            continue;
        }
        const GraphConnection* wire = nullptr;
        for (const SharedObjectPtr<GraphConnection>& connection : m_Graph->Connections) {
            if (connection && connection->UsesInput(InNode.NodeId, pin->Name)) {
                wire = connection.Get();
            }
        }
        if (!wire) {
            continue;
        }
        const EventGraphNode* source = Cast<EventGraphNode>(m_Graph->FindNode(wire->FromNodeId));
        if (!source) {
            continue;
        }
        if (source->IsPure() && !Evaluate(*source)) {
            return false;
        }
        const EventGraphPinValue from = source->GetPinValue(GetFrame(*source), wire->FromPinName);
        const EventGraphPinValue to = InNode.GetPinValue(InFrame, pin->Name);
        if (!from.ValueProperty || !to.ValueProperty || from.ValueProperty->GetClass() != to.ValueProperty->GetClass()) {
            ReportError(InNode, "cannot read '" + pin->Name + "' from '" + source->GetTitle() + "'");
            return false;
        }
        to.ValueProperty->CopyRawValue(to.Data, from.Data);
    }
    return true;
}

bool EventGraphInstance::Evaluate(const EventGraphNode& InNode) {
    if (m_Evaluating.Contains(InNode.NodeId)) {
        ReportError(InNode, "its inputs depend on its own output");
        return false;
    }
    m_Evaluating.Add(InNode.NodeId);
    void* frame = GetFrame(InNode);
    const bool pulled = frame && PullInputs(InNode, frame);
    if (pulled) {
        InNode.Execute(*this, frame);
    }
    m_Evaluating.Remove(InNode.NodeId);
    return pulled;
}
