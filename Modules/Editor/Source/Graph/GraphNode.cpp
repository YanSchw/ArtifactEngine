#include "GraphNode.h"

Vec4 GraphNode::GetAccentColor() const {
    return Vec4(0.16f, 0.29f, 0.47f, 1.0f);
}

GraphPin* GraphNode::AddPin(const String& InName, const String& InTypeName, GraphPinDirection InDirection) {
    GraphPin* pin = Object::Create<GraphPin>();
    pin->Name = InName;
    pin->TypeName = InTypeName;
    pin->Direction = InDirection;
    Pins.Add(SharedObjectPtr<GraphPin>(pin));
    return pin;
}

GraphPin* GraphNode::AddInput(const String& InName, const String& InTypeName) {
    return AddPin(InName, InTypeName, GraphPinDirection::Input);
}

GraphPin* GraphNode::AddOutput(const String& InName, const String& InTypeName) {
    return AddPin(InName, InTypeName, GraphPinDirection::Output);
}

GraphPin* GraphNode::FindPin(const String& InName, GraphPinDirection InDirection) const {
    for (const SharedObjectPtr<GraphPin>& pin : Pins) {
        if (pin && pin->Direction == InDirection && pin->Name == InName) {
            return pin.Get();
        }
    }
    return nullptr;
}

Array<GraphPin*> GraphNode::GetPins(GraphPinDirection InDirection) const {
    Array<GraphPin*> result;
    for (const SharedObjectPtr<GraphPin>& pin : Pins) {
        if (pin && pin->Direction == InDirection) {
            result.Add(pin.Get());
        }
    }
    return result;
}
