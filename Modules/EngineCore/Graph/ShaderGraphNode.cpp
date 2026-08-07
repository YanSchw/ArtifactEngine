#include "ShaderGraphNode.h"

#include "Graph/NodeGraph.h"

#include <format>

String ShaderGraphNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    (void)InOutputPin;
    (void)InContext;
    return "0.0";
}

ShaderValueType ShaderGraphNode::GetOutputType(const GraphPin& InOutputPin) const {
    ShaderValueType type = ShaderValueType::Float;
    ShaderValue::ParseGlslType(InOutputPin.TypeName, type);
    return type;
}

Vec4 ShaderGraphNode::GetAccentColor() const {
    const String category = GetCategory();
    if (category == "Constants") return Vec4(0.12f, 0.31f, 0.27f, 1.0f);
    if (category == "Math")      return Vec4(0.18f, 0.32f, 0.19f, 1.0f);
    if (category == "Input")     return Vec4(0.24f, 0.18f, 0.39f, 1.0f);
    if (category == "Output")    return Vec4(0.36f, 0.15f, 0.15f, 1.0f);
    return Vec4(0.11f, 0.24f, 0.40f, 1.0f);
}

bool ShaderGraphContext::FindUpstream(const GraphNode& InNode, const String& InPinName,
                                      const GraphNode*& OutNode, const GraphPin*& OutPin) const {
    for (const SharedObjectPtr<GraphConnection>& connection : m_Graph.Connections) {
        if (!connection || !connection->UsesInput(InNode.NodeId, InPinName)) {
            continue;
        }
        GraphNode* fromNode = m_Graph.FindNode(connection->FromNodeId);
        GraphPin* fromPin = fromNode ? fromNode->FindPin(connection->FromPinName, GraphPinDirection::Output) : nullptr;
        if (!fromPin) {
            continue;
        }
        OutNode = fromNode;
        OutPin = fromPin;
        return true;
    }
    return false;
}

bool ShaderGraphContext::HasInput(const GraphNode& InNode, const String& InPinName) const {
    const GraphNode* node = nullptr;
    const GraphPin* pin = nullptr;
    return FindUpstream(InNode, InPinName, node, pin);
}

String ShaderGraphContext::ReadInput(const GraphNode& InNode, const String& InPinName, ShaderValueType InExpectedType) {
    const GraphNode* fromNode = nullptr;
    const GraphPin* fromPin = nullptr;

    if (!FindUpstream(InNode, InPinName, fromNode, fromPin)) {
        const GraphPin* pin = InNode.FindPin(InPinName, GraphPinDirection::Input);
        return ShaderValue::Literal(pin ? pin->DefaultValue : Vec4(0.0f), InExpectedType);
    }

    const ShaderGraphNode* shaderNode = Cast<ShaderGraphNode>(fromNode);
    if (!shaderNode) {
        AddError(std::format("node '{0}' cannot contribute to a shader", fromNode->GetTitle()));
        return ShaderValue::Literal(Vec4(0.0f), InExpectedType);
    }

    const ShaderValueType sourceType = shaderNode->GetOutputType(*fromPin);
    const String cacheKey = std::format("{0}:{1}", fromNode->NodeId, fromPin->Name);

    if (m_Cache.ContainsKey(cacheKey)) {
        return ShaderValue::Convert(m_Cache[cacheKey], sourceType, InExpectedType);
    }

    if (m_Visiting.Contains(fromNode->NodeId)) {
        AddError(std::format("cycle through node '{0}'", fromNode->GetTitle()));
        return ShaderValue::Literal(Vec4(0.0f), InExpectedType);
    }

    m_Visiting.Add(fromNode->NodeId);
    const String hoisted = Hoist(shaderNode->Emit(*fromPin, *this), sourceType);
    m_Visiting.RemoveAt(m_Visiting.IndexOf(fromNode->NodeId));

    m_Cache[cacheKey] = hoisted;
    return ShaderValue::Convert(hoisted, sourceType, InExpectedType);
}

void ShaderGraphContext::BeginProperty() {
    m_Prelude.clear();
    m_Cache.Clear();
    m_Visiting.Clear();
}

String ShaderGraphContext::Hoist(const String& InExpression, ShaderValueType InType) {
    const String name = std::format("sg_{0}", m_NextTemp++);
    m_Prelude += std::format("    {0} {1} = {2};\n",
                             ShaderValue::GetGlslType(InType), name, InExpression);
    return name;
}
