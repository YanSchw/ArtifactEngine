#include "EventGraphNodes.h"
#include "EventGraph.h"
#include "EventGraphInstance.h"
#include "GameFramework/Node.h"

struct EventTickFrame {
    float DeltaSeconds = 0.0f;
};

struct ObjectFrame {
    WeakObjectPtr<Object> Value;
};

struct CastFrame {
    WeakObjectPtr<Object> Input;
    WeakObjectPtr<Object> Result;
};

struct BranchFrame {
    bool Condition = false;
};

struct CallFrame {
    WeakObjectPtr<Object> Target;
    std::shared_ptr<void> Params;
};

static FloatProperty s_DeltaSecondsProperty("DeltaSeconds", offsetof(EventTickFrame, DeltaSeconds), false);
static WeakObjectPtrProperty s_ObjectValueProperty("Value", offsetof(ObjectFrame, Value), Class("Object"));
static WeakObjectPtrProperty s_CastInputProperty("Object", offsetof(CastFrame, Input), Class("Object"));
static WeakObjectPtrProperty s_CastResultProperty("Result", offsetof(CastFrame, Result), Class("Object"));
static BoolProperty s_ConditionProperty("Condition", offsetof(BranchFrame, Condition));
static WeakObjectPtrProperty s_TargetProperty("Target", offsetof(CallFrame, Target), Class("Object"));

static Vec4 Accent(uint32_t InHex) {
    return Vec4((float)((InHex >> 16) & 0xFF), (float)((InHex >> 8) & 0xFF), (float)(InHex & 0xFF), 255.0f) / 255.0f;
}

static EventGraphPinValue ValueOf(Property& InProperty, void* InFrame) {
    return { &InProperty, InProperty.GetValuePtr(InFrame) };
}

Vec4 EventNode::GetAccentColor() const {
    return Accent(0x8C1D1D);
}

void EventBeginPlayNode::ConstructPins() {
    AddExecOutput();
}

void EventEndPlayNode::ConstructPins() {
    AddExecOutput();
}

void EventTickNode::ConstructPins() {
    AddExecOutput();
    AddValueOutput(&s_DeltaSecondsProperty);
}

std::shared_ptr<void> EventTickNode::MakeFrame() const {
    return std::make_shared<EventTickFrame>();
}

EventGraphPinValue EventTickNode::GetPinValue(void* InFrame, const String& InPinName) const {
    return InPinName == s_DeltaSecondsProperty.Name ? ValueOf(s_DeltaSecondsProperty, InFrame) : EventGraphPinValue();
}

DelegateProperty* DelegateEventNode::FindDelegate() const {
    return Cast<DelegateProperty>(Property::FindTypeProperty(Class(OwnerClass), DelegateName));
}

void DelegateEventNode::ConstructPins() {
    AddExecOutput();
    if (DelegateProperty* delegate = FindDelegate()) {
        for (Property* argument : delegate->SignatureFunction->Outputs) {
            AddValueOutput(argument);
        }
    }
}

String DelegateEventNode::GetTitle() const {
    const String title = EventGraph::PrettyName(DelegateName);
    return NodeName.empty() ? title : title + " (" + NodeName + ")";
}

std::shared_ptr<void> DelegateEventNode::MakeFrame() const {
    DelegateProperty* delegate = FindDelegate();
    return delegate ? delegate->SignatureFunction->MakeParams() : nullptr;
}

EventGraphPinValue DelegateEventNode::GetPinValue(void* InFrame, const String& InPinName) const {
    DelegateProperty* delegate = FindDelegate();
    Property* argument = delegate ? delegate->SignatureFunction->FindOutput(InPinName) : nullptr;
    return argument ? ValueOf(*argument, InFrame) : EventGraphPinValue();
}

void DelegateEventNode::Start(EventGraphInstance& InInstance) const {
    Node* target = NodeName.empty() ? InInstance.GetSelf() : InInstance.FindNode(NodeName);
    DelegateProperty* delegate = target ? Cast<DelegateProperty>(Property::FindTypeProperty(target->GetClass(), DelegateName)) : nullptr;
    if (!delegate) {
        InInstance.ReportError(*this, target ? "'" + target->GetName() + "' has no delegate '" + DelegateName + "'"
                                             : "there is no node named '" + NodeName + "'");
        return;
    }
    InInstance.BindDelegate(*this, *delegate, *target);
}

Function* CallFunctionNode::FindFunction() const {
    return Function::FindFunction(Class(FunctionClass), FunctionName);
}

void CallFunctionNode::ConstructPins() {
    Function* function = FindFunction();
    if (!function) {
        return;
    }
    if (!function->IsPure) {
        AddExecInput();
        AddExecOutput();
    }
    if (!function->IsStatic) {
        AddInput(TargetPin, EventGraph::ObjectPinType(Class(FunctionClass)));
    }
    for (Property* input : function->Inputs) {
        AddValueInput(input);
    }
    for (Property* output : function->Outputs) {
        AddValueOutput(output);
    }
}

String CallFunctionNode::GetTitle() const {
    return FindFunction() ? EventGraph::PrettyName(FunctionName) : "Unknown function " + FunctionClass + "::" + FunctionName;
}

Vec4 CallFunctionNode::GetAccentColor() const {
    return IsPure() ? Accent(0x2F6B3A) : Accent(0x1F4E8C);
}

bool CallFunctionNode::IsPure() const {
    Function* function = FindFunction();
    return function && function->IsPure;
}

std::shared_ptr<void> CallFunctionNode::MakeFrame() const {
    Function* function = FindFunction();
    if (!function) {
        return nullptr;
    }
    std::shared_ptr<CallFrame> frame = std::make_shared<CallFrame>();
    frame->Params = function->MakeParams();
    return frame;
}

EventGraphPinValue CallFunctionNode::GetPinValue(void* InFrame, const String& InPinName) const {
    CallFrame* frame = (CallFrame*)InFrame;
    if (InPinName == TargetPin) {
        return ValueOf(s_TargetProperty, frame);
    }
    Function* function = FindFunction();
    Property* property = function ? function->FindInput(InPinName) : nullptr;
    property = property ? property : function ? function->FindOutput(InPinName) : nullptr;
    return property ? EventGraphPinValue{ property, property->GetValuePtr(frame->Params.get()) } : EventGraphPinValue();
}

String CallFunctionNode::Execute(EventGraphInstance& InInstance, void* InFrame) const {
    Function* function = FindFunction();
    if (!function) {
        InInstance.ReportError(*this, "the function no longer exists");
        return String();
    }
    CallFrame* frame = (CallFrame*)InFrame;
    Object* target = function->IsStatic ? nullptr
                   : InInstance.IsPinConnected(*this, TargetPin) ? frame->Target.Get() : (Object*)InInstance.GetSelf();
    if (!function->CanCallOn(target)) {
        InInstance.ReportError(*this, target ? "the target is not a " + FunctionClass : "the target is None");
        return String();
    }
    function->Invoke(target, frame->Params.get());
    return function->IsPure ? String() : ThenPin;
}

String CallFunctionNode::GetPinValueText(const GraphPin& InPin) const {
    return InPin.Name == TargetPin ? String("self") : EventGraphNode::GetPinValueText(InPin);
}

void CastNode::ConstructPins() {
    AddExecInput();
    AddInput(ObjectPin, EventGraph::ObjectPinType(Object::StaticClass()));
    AddExecOutput(SuccessPin);
    AddExecOutput(FailedPin);
    AddOutput(ResultPin, EventGraph::ObjectPinType(Class(TargetClass)));
}

Vec4 CastNode::GetAccentColor() const {
    return Accent(0x2A6E7A);
}

String CastNode::GetPinLabel(const GraphPin& InPin) const {
    return InPin.Name == ResultPin ? "As " + TargetClass : EventGraphNode::GetPinLabel(InPin);
}

std::shared_ptr<void> CastNode::MakeFrame() const {
    return std::make_shared<CastFrame>();
}

EventGraphPinValue CastNode::GetPinValue(void* InFrame, const String& InPinName) const {
    if (InPinName == ObjectPin) {
        return ValueOf(s_CastInputProperty, InFrame);
    }
    return InPinName == ResultPin ? ValueOf(s_CastResultProperty, InFrame) : EventGraphPinValue();
}

String CastNode::Execute(EventGraphInstance& InInstance, void* InFrame) const {
    (void)InInstance;
    CastFrame* frame = (CastFrame*)InFrame;
    Object* input = frame->Input.Get();
    const bool success = input && input->IsA(Class(TargetClass));
    frame->Result = success ? input : nullptr;
    return success ? SuccessPin : FailedPin;
}

void BranchNode::ConstructPins() {
    AddExecInput();
    AddValueInput(&s_ConditionProperty);
    AddExecOutput(TruePin);
    AddExecOutput(FalsePin);
}

Vec4 BranchNode::GetAccentColor() const {
    return Accent(0x4A4A4A);
}

std::shared_ptr<void> BranchNode::MakeFrame() const {
    return std::make_shared<BranchFrame>();
}

EventGraphPinValue BranchNode::GetPinValue(void* InFrame, const String& InPinName) const {
    return InPinName == ConditionPin ? ValueOf(s_ConditionProperty, InFrame) : EventGraphPinValue();
}

String BranchNode::Execute(EventGraphInstance& InInstance, void* InFrame) const {
    (void)InInstance;
    return ((BranchFrame*)InFrame)->Condition ? TruePin : FalsePin;
}

void SelfNode::ConstructPins() {
    AddOutput(SelfPin, EventGraph::ObjectPinType(Class(SelfClass)));
}

Vec4 SelfNode::GetAccentColor() const {
    return Accent(0x3D3D5C);
}

std::shared_ptr<void> SelfNode::MakeFrame() const {
    return std::make_shared<ObjectFrame>();
}

EventGraphPinValue SelfNode::GetPinValue(void* InFrame, const String& InPinName) const {
    return InPinName == SelfPin ? ValueOf(s_ObjectValueProperty, InFrame) : EventGraphPinValue();
}

String SelfNode::Execute(EventGraphInstance& InInstance, void* InFrame) const {
    ((ObjectFrame*)InFrame)->Value = InInstance.GetSelf();
    return String();
}

void NodeReferenceNode::ConstructPins() {
    AddOutput(NodePin, EventGraph::ObjectPinType(Class(NodeClass)));
}

Vec4 NodeReferenceNode::GetAccentColor() const {
    return Accent(0x3D3D5C);
}

String NodeReferenceNode::GetPinLabel(const GraphPin& InPin) const {
    return InPin.Name == NodePin ? NodeClass : EventGraphNode::GetPinLabel(InPin);
}

std::shared_ptr<void> NodeReferenceNode::MakeFrame() const {
    return std::make_shared<ObjectFrame>();
}

EventGraphPinValue NodeReferenceNode::GetPinValue(void* InFrame, const String& InPinName) const {
    return InPinName == NodePin ? ValueOf(s_ObjectValueProperty, InFrame) : EventGraphPinValue();
}

String NodeReferenceNode::Execute(EventGraphInstance& InInstance, void* InFrame) const {
    Node* node = InInstance.FindNode(NodeName);
    if (!node) {
        InInstance.ReportError(*this, "there is no node named '" + NodeName + "'");
    }
    ((ObjectFrame*)InFrame)->Value = node;
    return String();
}
