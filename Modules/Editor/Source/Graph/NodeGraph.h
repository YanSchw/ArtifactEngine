#pragma once
#include "GraphNode.h"
#include "GraphConnection.h"
#include "NodeGraph.gen.h"

/** A serializable document of GraphNodes wired together by GraphConnections. */
class NodeGraph : public Object {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    String GraphName = "Node Graph";

    PROPERTY()
    uint64_t NextNodeId = 1;

    PROPERTY()
    Array<SharedObjectPtr<GraphNode>> Nodes;

    PROPERTY()
    Array<SharedObjectPtr<GraphConnection>> Connections;

    GraphNode* CreateNode(const Class& InNodeClass, const Vec2& InPosition);
    template<typename T>
    T* CreateNode(const Vec2& InPosition) {
        return Cast<T>(CreateNode(T::StaticClass(), InPosition));
    }
    /** Removes the node and every connection touching it. */
    void RemoveNode(uint64_t InNodeId);
    GraphNode* FindNode(uint64_t InNodeId) const;
    /** Repaints last, i.e. above its siblings, in the editor. */
    void BringToFront(GraphNode& InNode);

    /** Base rules: distinct nodes, an output into an input, compatible types. */
    virtual bool CanConnect(const GraphNode& InNodeA, const GraphPin& InPinA,
                            const GraphNode& InNodeB, const GraphPin& InPinB, String& OutReason) const;
    virtual bool ArePinTypesCompatible(const String& InOutputType, const String& InInputType) const {
        return InOutputType == InInputType;
    }

    /** Wires two pins (either order); an input pin's existing connection is replaced, as inputs
     *  accept a single wire. Returns null with a log entry when CanConnect rejects the pair. */
    GraphConnection* Connect(GraphNode& InNodeA, GraphPin& InPinA, GraphNode& InNodeB, GraphPin& InPinB);
    void BreakPinConnections(const GraphNode& InNode, const GraphPin& InPin);
    bool IsPinConnected(const GraphNode& InNode, const GraphPin& InPin) const;

    bool SaveToFile(const String& InFilePath) const;
    static SharedObjectPtr<NodeGraph> LoadFromFile(const String& InFilePath);
};
