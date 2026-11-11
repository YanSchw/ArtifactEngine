#pragma once
#include "Graph/GraphNode.h"
#include "Common/Map.h"
#include <memory>
#include "EventGraphNode.gen.h"

class EventGraphInstance;

struct EventGraphPinValue {
    Property* ValueProperty = nullptr;
    void* Data = nullptr;
};

class EventGraphNode : public GraphNode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    Map<String, String> PinDefaults;

    inline static const String ExecutePin = "Execute";
    inline static const String ThenPin = "Then";

    virtual bool IsPure() const { return false; }
    virtual bool IsResolved() const { return true; }
    virtual std::shared_ptr<void> MakeFrame() const { return nullptr; }
    virtual EventGraphPinValue GetPinValue(void* InFrame, const String& InPinName) const;
    virtual String Execute(EventGraphInstance& InInstance, void* InFrame) const;
    virtual void Start(EventGraphInstance& InInstance) const { (void)InInstance; }

    std::shared_ptr<void> MakeFrameWithDefaults() const;
    void* GetDefaultValuePtr(const String& InPinName);
    void CommitDefaultValue(const String& InPinName);
    bool HasEditableDefault(const GraphPin& InPin) const;

    virtual String GetCategory() const override { return "Event Graph"; }
    virtual bool IsUserCreatable() const override { return false; }
    virtual String GetPinLabel(const GraphPin& InPin) const override;
    virtual String GetPinValueText(const GraphPin& InPin) const override;

protected:
    GraphPin* AddExecInput(const String& InName = ExecutePin);
    GraphPin* AddExecOutput(const String& InName = ThenPin);
    GraphPin* AddValueInput(Property* InProperty);
    GraphPin* AddValueOutput(Property* InProperty);

private:
    std::shared_ptr<void> m_DefaultsFrame;
    mutable Map<String, String> m_ValueTexts;
};
