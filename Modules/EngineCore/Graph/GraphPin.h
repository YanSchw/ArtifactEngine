#pragma once
#include "Object/Object.h"
#include "Object/Enum.h"
#include "Common/Types.h"
#include "GraphPin.gen.h"

ARTIFACT_ENUM();
enum class GraphPinDirection : uint8_t {
    Input = 0,
    Output = 1
};

class GraphPin : public Object {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    String Name;

    PROPERTY()
    GraphPinDirection Direction = GraphPinDirection::Input;

    /** The value/signal type this pin carries (e.g. "Float", "Color"). Free-form so future graph
     *  schemas can introduce types without touching this class; NodeGraph::ArePinTypesCompatible
     *  and GraphEditorStyle::PinColor give it meaning. */
    PROPERTY()
    String TypeName = "Float";

    PROPERTY()
    Vec4 DefaultValue = Vec4(0.0f);

    bool IsInput() const { return Direction == GraphPinDirection::Input; }
    bool IsOutput() const { return Direction == GraphPinDirection::Output; }
};
