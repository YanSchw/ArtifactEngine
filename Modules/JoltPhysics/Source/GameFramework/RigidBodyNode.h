#pragma once
#include "PhysicsBodyNode.h"
#include "RigidBodyNode.gen.h"

/** A body whose collider is every ShapeNode below it. */
class RigidBodyNode : public PhysicsBodyNode {
public:
    ARTIFACT_CLASS();

    RigidBodyNode();

protected:
    virtual void CollectShapes(Array<PhysicsShapeDesc>& OutShapes) const override;
};
