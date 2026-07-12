#include "GraphExampleNodes.h"

void GraphTimeNode::ConstructPins() {
    AddOutput("Seconds", "Float");
}

void GraphFloatNode::ConstructPins() {
    AddOutput("Value", "Float");
}

void GraphAddNode::ConstructPins() {
    AddInput("A", "Float");
    AddInput("B", "Float");
    AddOutput("Result", "Float");
}

void GraphMultiplyNode::ConstructPins() {
    AddInput("A", "Float");
    AddInput("B", "Float");
    AddOutput("Result", "Float");
}

void GraphLerpNode::ConstructPins() {
    AddInput("A", "Float");
    AddInput("B", "Float");
    AddInput("Alpha", "Float");
    AddOutput("Result", "Float");
}

void GraphMakeColorNode::ConstructPins() {
    AddInput("R", "Float");
    AddInput("G", "Float");
    AddInput("B", "Float");
    AddOutput("Color", "Color");
}

void GraphOutputNode::ConstructPins() {
    AddInput("Color", "Color");
    AddInput("Emissive", "Color");
    AddInput("Roughness", "Float");
}

SharedObjectPtr<NodeGraph> BuildExampleNodeGraph() {
    SharedObjectPtr<NodeGraph> graph(Object::Create<NodeGraph>());
    graph->GraphName = "Example Graph";

    GraphTimeNode* time = graph->CreateNode<GraphTimeNode>(Vec2(-608.0f, -192.0f));
    GraphFloatNode* speed = graph->CreateNode<GraphFloatNode>(Vec2(-608.0f, -64.0f));
    GraphMultiplyNode* multiply = graph->CreateNode<GraphMultiplyNode>(Vec2(-384.0f, -144.0f));
    GraphFloatNode* dark = graph->CreateNode<GraphFloatNode>(Vec2(-384.0f, 32.0f));
    GraphFloatNode* bright = graph->CreateNode<GraphFloatNode>(Vec2(-384.0f, 144.0f));
    GraphLerpNode* lerp = graph->CreateNode<GraphLerpNode>(Vec2(-144.0f, -16.0f));
    GraphMakeColorNode* makeColor = graph->CreateNode<GraphMakeColorNode>(Vec2(96.0f, -96.0f));
    GraphFloatNode* roughness = graph->CreateNode<GraphFloatNode>(Vec2(96.0f, 96.0f));
    GraphOutputNode* output = graph->CreateNode<GraphOutputNode>(Vec2(336.0f, -32.0f));

    const auto connect = [&graph](GraphNode* InFrom, const String& InOutput, GraphNode* InTo, const String& InInput) {
        GraphPin* fromPin = InFrom->FindPin(InOutput, GraphPinDirection::Output);
        GraphPin* toPin = InTo->FindPin(InInput, GraphPinDirection::Input);
        if (fromPin && toPin) {
            graph->Connect(*InFrom, *fromPin, *InTo, *toPin);
        }
    };
    connect(time, "Seconds", multiply, "A");
    connect(speed, "Value", multiply, "B");
    connect(multiply, "Result", lerp, "Alpha");
    connect(dark, "Value", lerp, "A");
    connect(bright, "Value", lerp, "B");
    connect(lerp, "Result", makeColor, "R");
    connect(lerp, "Result", makeColor, "G");
    connect(multiply, "Result", makeColor, "B");
    connect(makeColor, "Color", output, "Color");
    connect(roughness, "Value", output, "Roughness");

    return graph;
}
