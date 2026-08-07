#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "GraphPin.h"
#include "GraphNode.gen.h"

class GraphNode : public Object {
public:
    ARTIFACT_CLASS();

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
    virtual String GetCategory() const { return "Common"; }
    virtual Vec4 GetAccentColor() const;
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
