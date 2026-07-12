#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "GraphPin.h"
#include "GraphNode.gen.h"

/** A single node in a NodeGraph: a title bar, input pins on the left and output pins on the
 *  right. Subclasses declare their pins in ConstructPins() and describe themselves through the
 *  Get* virtuals; everything that defines a placed node instance lives in PROPERTY fields, so a
 *  graph serializes without any per-subclass code.
 *
 *  ConstructPins() only runs when a node is first created (NodeGraph::CreateNode); deserialized
 *  nodes restore their pins from data, which lets saved graphs keep per-instance pins and
 *  survive node-definition changes. */
class GraphNode : public Object {
public:
    ARTIFACT_CLASS();

    /** Identifies this node within its graph (connections reference it); assigned by NodeGraph. */
    PROPERTY()
    uint64_t NodeId = 0;

    PROPERTY()
    float PositionX = 0.0f;

    PROPERTY()
    float PositionY = 0.0f;

    PROPERTY()
    Array<SharedObjectPtr<GraphPin>> Pins;

    virtual void ConstructPins() { }

    virtual String GetTitle() const { return GetClass().Name; }
    /** Section this node is listed under in the editor's creation menu. */
    virtual String GetCategory() const { return "Common"; }
    /** Tint of the node's title bar in the editor. */
    virtual Vec4 GetAccentColor() const;
    /** Excludes helper/internal nodes from the editor's creation menu. */
    virtual bool IsUserCreatable() const { return true; }

    Vec2 GetPosition() const { return Vec2(PositionX, PositionY); }
    void SetPosition(const Vec2& InPosition) { PositionX = InPosition.x; PositionY = InPosition.y; }

    GraphPin* AddInput(const String& InName, const String& InTypeName);
    GraphPin* AddOutput(const String& InName, const String& InTypeName);
    GraphPin* FindPin(const String& InName, GraphPinDirection InDirection) const;
    Array<GraphPin*> GetPins(GraphPinDirection InDirection) const;

private:
    GraphPin* AddPin(const String& InName, const String& InTypeName, GraphPinDirection InDirection);
};
