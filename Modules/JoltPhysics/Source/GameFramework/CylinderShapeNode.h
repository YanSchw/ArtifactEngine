#pragma once
#include "ShapeNode.h"
#include "CylinderShapeNode.gen.h"

class CylinderShapeNode : public ShapeNode {
public:
    ARTIFACT_CLASS();

    virtual PhysicsShapeDesc GetShapeDesc() const override {
        PhysicsShapeDesc shape;
        shape.Type = PhysicsShapeType::Cylinder;
        shape.Radius = m_Radius;
        shape.HalfHeight = m_HalfHeight;
        return shape;
    }

    FUNCTION(Pure)
    float GetRadius() const { return m_Radius; }
    FUNCTION()
    void SetRadius(float InRadius) {
        m_Radius = InRadius;
        RefreshShape();
    }

    FUNCTION(Pure)
    float GetHalfHeight() const { return m_HalfHeight; }
    FUNCTION()
    void SetHalfHeight(float InHalfHeight) {
        m_HalfHeight = InHalfHeight;
        RefreshShape();
    }

private:
    PROPERTY()
    float m_Radius = 0.5f;

    PROPERTY()
    float m_HalfHeight = 0.5f;
};
