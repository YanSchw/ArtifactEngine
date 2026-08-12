#pragma once
#include "ShapeNode.h"
#include "SphereShapeNode.gen.h"

class SphereShapeNode : public ShapeNode {
public:
    ARTIFACT_CLASS();

    virtual PhysicsShapeDesc GetShapeDesc() const override {
        PhysicsShapeDesc shape;
        shape.Type = PhysicsShapeType::Sphere;
        shape.Radius = m_Radius;
        return shape;
    }

    float GetRadius() const { return m_Radius; }
    void SetRadius(float InRadius) {
        m_Radius = InRadius;
        RefreshShape();
    }

private:
    PROPERTY()
    float m_Radius = 0.5f;
};
