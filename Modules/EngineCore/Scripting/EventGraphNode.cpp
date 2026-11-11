#include "EventGraphNode.h"
#include "EventGraph.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include <cstdio>

EventGraphPinValue EventGraphNode::GetPinValue(void* InFrame, const String& InPinName) const {
    (void)InFrame;
    (void)InPinName;
    return EventGraphPinValue();
}

String EventGraphNode::Execute(EventGraphInstance& InInstance, void* InFrame) const {
    (void)InInstance;
    (void)InFrame;
    return IsPure() ? String() : ThenPin;
}

std::shared_ptr<void> EventGraphNode::MakeFrameWithDefaults() const {
    std::shared_ptr<void> frame = MakeFrame();
    if (!frame) {
        return frame;
    }
    for (const auto& [pinName, text] : PinDefaults) {
        const EventGraphPinValue value = GetPinValue(frame.get(), pinName);
        const nlohmann::json json = nlohmann::json::parse(text, nullptr, false);
        if (value.ValueProperty && !json.is_discarded()) {
            try {
                JsonSerializer::DeserializeProperty(value.ValueProperty, value.Data, json);
            } catch (const nlohmann::json::exception&) {
            }
        }
    }
    return frame;
}

void* EventGraphNode::GetDefaultValuePtr(const String& InPinName) {
    m_DefaultsFrame = MakeFrameWithDefaults();
    return m_DefaultsFrame ? GetPinValue(m_DefaultsFrame.get(), InPinName).Data : nullptr;
}

void EventGraphNode::CommitDefaultValue(const String& InPinName) {
    if (!m_DefaultsFrame) {
        return;
    }
    const EventGraphPinValue value = GetPinValue(m_DefaultsFrame.get(), InPinName);
    if (value.ValueProperty) {
        PinDefaults[InPinName] = JsonSerializer::SerializeProperty(value.ValueProperty, value.Data).dump();
        m_ValueTexts.Clear();
    }
}

bool EventGraphNode::HasEditableDefault(const GraphPin& InPin) const {
    if (!InPin.IsInput() || InPin.TypeName == EventGraph::ExecType || InPin.TypeName.starts_with("Object:")) {
        return false;
    }
    std::shared_ptr<void> frame = MakeFrame();
    return frame && GetPinValue(frame.get(), InPin.Name).ValueProperty != nullptr;
}

String EventGraphNode::GetPinLabel(const GraphPin& InPin) const {
    if (InPin.TypeName == EventGraph::ExecType && (InPin.Name == ExecutePin || InPin.Name == ThenPin)) {
        return String();
    }
    return EventGraph::PrettyName(InPin.Name);
}

static String FormatValue(const nlohmann::json& InValue) {
    if (InValue.is_string()) {
        return InValue.get<String>();
    }
    if (InValue.is_number_float()) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%g", InValue.get<double>());
        return buffer;
    }
    if (InValue.is_object() || InValue.is_array()) {
        String text;
        for (const nlohmann::json& element : InValue) {
            text += (text.empty() ? "" : ", ") + FormatValue(element);
        }
        return text;
    }
    return InValue.is_null() ? String("None") : InValue.dump();
}

String EventGraphNode::GetPinValueText(const GraphPin& InPin) const {
    const String key = InPin.Name + (InPin.IsInput() ? ">" : "<") + InPin.TypeName;
    if (const String* cached = m_ValueTexts.Find(key)) {
        return *cached;
    }
    String text;
    if (HasEditableDefault(InPin)) {
        const std::shared_ptr<void> frame = MakeFrameWithDefaults();
        const EventGraphPinValue value = GetPinValue(frame.get(), InPin.Name);
        text = FormatValue(JsonSerializer::SerializeProperty(value.ValueProperty, value.Data));
    }
    m_ValueTexts[key] = text;
    return text;
}

GraphPin* EventGraphNode::AddExecInput(const String& InName) {
    return AddInput(InName, EventGraph::ExecType);
}

GraphPin* EventGraphNode::AddExecOutput(const String& InName) {
    return AddOutput(InName, EventGraph::ExecType);
}

GraphPin* EventGraphNode::AddValueInput(Property* InProperty) {
    return AddInput(InProperty->Name, EventGraph::PinTypeOf(InProperty));
}

GraphPin* EventGraphNode::AddValueOutput(Property* InProperty) {
    return AddOutput(InProperty->Name, EventGraph::PinTypeOf(InProperty));
}
