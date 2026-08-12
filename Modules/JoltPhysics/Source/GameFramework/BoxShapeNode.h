#pragma once
#include "ShapeNode.h"
#include "BoxShapeNode.gen.h"

class BoxShapeNode : public ShapeNode {
public:
    ARTIFACT_CLASS();

    virtual PhysicsShapeDesc GetShapeDesc() const override {
        PhysicsShapeDesc shape;
        shape.Type = PhysicsShapeType::Box;
        shape.HalfExtents = m_HalfExtents;
        return shape;
    }

    Vec3 GetHalfExtents() const { return m_HalfExtents; }
    void SetHalfExtents(const Vec3& InHalfExtents) {
        m_HalfExtents = InHalfExtents;
        RefreshShape();
    }

private:
    PROPERTY()
    Vec3 m_HalfExtents = Vec3(0.5f);
};
