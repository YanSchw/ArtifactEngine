#pragma once
#include "NodeGraph.h"
#include "UI/EditorStyle.h"
#include "GraphExampleNodes.gen.h"

/** Placeholder node set exercising the graph editor (pins, types, categories, colors) until real
 *  schemas — material, visual scripting, animation — bring their own GraphNode subclasses. */

class GraphTimeNode : public GraphNode {
public:
    ARTIFACT_CLASS();
    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Time"; }
    virtual String GetCategory() const override { return "Input"; }
    virtual Vec4 GetAccentColor() const override { return HexColor(0x3D2E63); }
};

class GraphFloatNode : public GraphNode {
public:
    ARTIFACT_CLASS();
    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Float"; }
    virtual String GetCategory() const override { return "Constants"; }
    virtual Vec4 GetAccentColor() const override { return HexColor(0x1F4F45); }
};

class GraphAddNode : public GraphNode {
public:
    ARTIFACT_CLASS();
    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Add"; }
    virtual String GetCategory() const override { return "Math"; }
    virtual Vec4 GetAccentColor() const override { return HexColor(0x2E5231); }
};

class GraphMultiplyNode : public GraphNode {
public:
    ARTIFACT_CLASS();
    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Multiply"; }
    virtual String GetCategory() const override { return "Math"; }
    virtual Vec4 GetAccentColor() const override { return HexColor(0x2E5231); }
};

class GraphLerpNode : public GraphNode {
public:
    ARTIFACT_CLASS();
    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Lerp"; }
    virtual String GetCategory() const override { return "Math"; }
    virtual Vec4 GetAccentColor() const override { return HexColor(0x2E5231); }
};

class GraphMakeColorNode : public GraphNode {
public:
    ARTIFACT_CLASS();
    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Make Color"; }
    virtual String GetCategory() const override { return "Color"; }
    virtual Vec4 GetAccentColor() const override { return HexColor(0x1D3E66); }
};

class GraphOutputNode : public GraphNode {
public:
    ARTIFACT_CLASS();
    virtual void ConstructPins() override;
    virtual String GetTitle() const override { return "Surface Output"; }
    virtual String GetCategory() const override { return "Output"; }
    virtual Vec4 GetAccentColor() const override { return HexColor(0x5C2726); }
};

/** The graph the editor opens with when no saved document exists yet. */
SharedObjectPtr<NodeGraph> BuildExampleNodeGraph();
