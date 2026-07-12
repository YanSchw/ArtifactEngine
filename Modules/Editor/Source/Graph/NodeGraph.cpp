#include "NodeGraph.h"
#include "Core/Log.h"
#include "Serialization/Json.h"

#include <fstream>
#include <sstream>

GraphNode* NodeGraph::CreateNode(const Class& InNodeClass, const Vec2& InPosition) {
    GraphNode* node = Cast<GraphNode>(Object::Create(InNodeClass));
    if (!node) {
        AE_WARN("NodeGraph: '{0}' is not a creatable GraphNode class", InNodeClass.Name);
        return nullptr;
    }

    node->NodeId = NextNodeId++;
    node->SetPosition(InPosition);
    node->ConstructPins();
    Nodes.Add(SharedObjectPtr<GraphNode>(node));
    return node;
}

void NodeGraph::RemoveNode(uint64_t InNodeId) {
    for (int32_t i = Connections.Size() - 1; i >= 0; i--) {
        if (Connections[i] && Connections[i]->TouchesNode(InNodeId)) {
            Connections.RemoveAt(i);
        }
    }
    for (int32_t i = Nodes.Size() - 1; i >= 0; i--) {
        if (Nodes[i] && Nodes[i]->NodeId == InNodeId) {
            Nodes.RemoveAt(i);
        }
    }
}

GraphNode* NodeGraph::FindNode(uint64_t InNodeId) const {
    for (const SharedObjectPtr<GraphNode>& node : Nodes) {
        if (node && node->NodeId == InNodeId) {
            return node.Get();
        }
    }
    return nullptr;
}

void NodeGraph::BringToFront(GraphNode& InNode) {
    for (int32_t i = 0; i < Nodes.Size(); i++) {
        if (Nodes[i].Get() == &InNode) {
            SharedObjectPtr<GraphNode> keeper = Nodes[i];
            Nodes.RemoveAt(i);
            Nodes.Add(keeper);
            return;
        }
    }
}

bool NodeGraph::CanConnect(const GraphNode& InNodeA, const GraphPin& InPinA,
                           const GraphNode& InNodeB, const GraphPin& InPinB, String& OutReason) const {
    if (InNodeA.NodeId == InNodeB.NodeId) {
        OutReason = "Cannot connect a node to itself";
        return false;
    }
    if (InPinA.Direction == InPinB.Direction) {
        OutReason = InPinA.IsInput() ? "Cannot connect two inputs" : "Cannot connect two outputs";
        return false;
    }
    const GraphPin& output = InPinA.IsOutput() ? InPinA : InPinB;
    const GraphPin& input = InPinA.IsOutput() ? InPinB : InPinA;
    if (!ArePinTypesCompatible(output.TypeName, input.TypeName)) {
        OutReason = output.TypeName + " is not compatible with " + input.TypeName;
        return false;
    }
    return true;
}

GraphConnection* NodeGraph::Connect(GraphNode& InNodeA, GraphPin& InPinA, GraphNode& InNodeB, GraphPin& InPinB) {
    String reason;
    if (!CanConnect(InNodeA, InPinA, InNodeB, InPinB, reason)) {
        AE_INFO("NodeGraph: rejected connection - {0}", reason);
        return nullptr;
    }

    const bool aIsOutput = InPinA.IsOutput();
    GraphNode& fromNode = aIsOutput ? InNodeA : InNodeB;
    GraphPin& fromPin = aIsOutput ? InPinA : InPinB;
    GraphNode& toNode = aIsOutput ? InNodeB : InNodeA;
    GraphPin& toPin = aIsOutput ? InPinB : InPinA;

    BreakPinConnections(toNode, toPin);

    GraphConnection* connection = Object::Create<GraphConnection>();
    connection->FromNodeId = fromNode.NodeId;
    connection->FromPinName = fromPin.Name;
    connection->ToNodeId = toNode.NodeId;
    connection->ToPinName = toPin.Name;
    Connections.Add(SharedObjectPtr<GraphConnection>(connection));
    return connection;
}

void NodeGraph::BreakPinConnections(const GraphNode& InNode, const GraphPin& InPin) {
    for (int32_t i = Connections.Size() - 1; i >= 0; i--) {
        if (!Connections[i]) {
            continue;
        }
        const bool matches = InPin.IsOutput()
            ? Connections[i]->UsesOutput(InNode.NodeId, InPin.Name)
            : Connections[i]->UsesInput(InNode.NodeId, InPin.Name);
        if (matches) {
            Connections.RemoveAt(i);
        }
    }
}

bool NodeGraph::IsPinConnected(const GraphNode& InNode, const GraphPin& InPin) const {
    for (const SharedObjectPtr<GraphConnection>& connection : Connections) {
        if (!connection) {
            continue;
        }
        const bool matches = InPin.IsOutput()
            ? connection->UsesOutput(InNode.NodeId, InPin.Name)
            : connection->UsesInput(InNode.NodeId, InPin.Name);
        if (matches) {
            return true;
        }
    }
    return false;
}

bool NodeGraph::SaveToFile(const String& InFilePath) const {
    std::ofstream file(InFilePath, std::ios::binary);
    if (!file.is_open()) {
        AE_WARN("NodeGraph: could not open '{0}' for writing", InFilePath);
        return false;
    }
    file << JsonSerializer::SerializeObject(this);
    AE_INFO("NodeGraph: saved '{0}' to {1}", GraphName, InFilePath);
    return true;
}

SharedObjectPtr<NodeGraph> NodeGraph::LoadFromFile(const String& InFilePath) {
    std::ifstream file(InFilePath, std::ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    Object* object = JsonSerializer::DeserializeNewObject(buffer.str());
    NodeGraph* graph = Cast<NodeGraph>(object);
    if (!graph) {
        AE_WARN("NodeGraph: '{0}' does not contain a NodeGraph", InFilePath);
        delete object;
        return nullptr;
    }
    AE_INFO("NodeGraph: loaded '{0}' from {1}", graph->GraphName, InFilePath);
    return SharedObjectPtr<NodeGraph>(graph);
}
