#pragma once
#include "PhysicsBodyNode.h"
#include "ShapeNode.gen.h"

class RigidBodyNode;

/** A single collider. On its own it is a body of its own; below a RigidBodyNode it only contributes
 *  its shape to that body. */
class ShapeNode : public PhysicsBodyNode {
public:
    ARTIFACT_CLASS();

    virtual PhysicsShapeDesc GetShapeDesc() const = 0;

    RigidBodyNode* GetOwningRigidBody() const;

protected:
    virtual void CollectShapes(Array<PhysicsShapeDesc>& OutShapes) const override;
    virtual bool ShouldCreateBody() const override;

    void RefreshShape();
};
