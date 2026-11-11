#pragma once
#include "EventGraphNode.h"
#include "EventGraphNodes.gen.h"

struct Function;
struct DelegateProperty;

class EventNode : public EventGraphNode {
public:
    ARTIFACT_CLASS();

    virtual Vec4 GetAccentColor() const override;
};

class EventBeginPlayNode : public EventNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Event BeginPlay"; }
};

class EventEndPlayNode : public EventNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Event EndPlay"; }
};

class EventTickNode : public EventNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Event Tick"; }
    virtual std::shared_ptr<void> MakeFrame() const override;
    virtual EventGraphPinValue GetPinValue(void* InFrame, const String& InPinName) const override;
};

class DelegateEventNode : public EventNode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    String NodeName;

    PROPERTY()
    String DelegateName;

    PROPERTY()
    String OwnerClass;

    DelegateProperty* FindDelegate() const;

    virtual void ConstructPins() override;
    virtual String GetTitle() const override;
    virtual bool IsResolved() const override { return FindDelegate() != nullptr; }
    virtual std::shared_ptr<void> MakeFrame() const override;
    virtual EventGraphPinValue GetPinValue(void* InFrame, const String& InPinName) const override;
    virtual void Start(EventGraphInstance& InInstance) const override;
};

class CallFunctionNode : public EventGraphNode {
public:
    ARTIFACT_CLASS();

    inline static const String TargetPin = "Target";

    PROPERTY()
    String FunctionClass;

    PROPERTY()
    String FunctionName;

    Function* FindFunction() const;

    virtual void ConstructPins() override;
    virtual String GetTitle() const override;
    virtual Vec4 GetAccentColor() const override;
    virtual bool IsPure() const override;
    virtual bool IsResolved() const override { return FindFunction() != nullptr; }
    virtual std::shared_ptr<void> MakeFrame() const override;
    virtual EventGraphPinValue GetPinValue(void* InFrame, const String& InPinName) const override;
    virtual String Execute(EventGraphInstance& InInstance, void* InFrame) const override;
    virtual String GetPinValueText(const GraphPin& InPin) const override;
};

class CastNode : public EventGraphNode {
public:
    ARTIFACT_CLASS();

    inline static const String ObjectPin = "Object";
    inline static const String ResultPin = "Result";
    inline static const String SuccessPin = "Success";
    inline static const String FailedPin = "Failed";

    PROPERTY()
    String TargetClass = "Node";

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Cast To " + TargetClass; }
    virtual Vec4 GetAccentColor() const override;
    virtual String GetPinLabel(const GraphPin& InPin) const override;
    virtual std::shared_ptr<void> MakeFrame() const override;
    virtual EventGraphPinValue GetPinValue(void* InFrame, const String& InPinName) const override;
    virtual String Execute(EventGraphInstance& InInstance, void* InFrame) const override;
};

class BranchNode : public EventGraphNode {
public:
    ARTIFACT_CLASS();

    inline static const String ConditionPin = "Condition";
    inline static const String TruePin = "True";
    inline static const String FalsePin = "False";

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Branch"; }
    virtual Vec4 GetAccentColor() const override;
    virtual std::shared_ptr<void> MakeFrame() const override;
    virtual EventGraphPinValue GetPinValue(void* InFrame, const String& InPinName) const override;
    virtual String Execute(EventGraphInstance& InInstance, void* InFrame) const override;
};

class SelfNode : public EventGraphNode {
public:
    ARTIFACT_CLASS();

    inline static const String SelfPin = "Self";

    PROPERTY()
    String SelfClass = "Node";

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Self"; }
    virtual Vec4 GetAccentColor() const override;
    virtual bool IsPure() const override { return true; }
    virtual std::shared_ptr<void> MakeFrame() const override;
    virtual EventGraphPinValue GetPinValue(void* InFrame, const String& InPinName) const override;
    virtual String Execute(EventGraphInstance& InInstance, void* InFrame) const override;
};

class NodeReferenceNode : public EventGraphNode {
public:
    ARTIFACT_CLASS();

    inline static const String NodePin = "Node";

    PROPERTY()
    String NodeName;

    PROPERTY()
    String NodeClass = "Node";

    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return NodeName; }
    virtual Vec4 GetAccentColor() const override;
    virtual bool IsPure() const override { return true; }
    virtual String GetPinLabel(const GraphPin& InPin) const override;
    virtual std::shared_ptr<void> MakeFrame() const override;
    virtual EventGraphPinValue GetPinValue(void* InFrame, const String& InPinName) const override;
    virtual String Execute(EventGraphInstance& InInstance, void* InFrame) const override;
};
