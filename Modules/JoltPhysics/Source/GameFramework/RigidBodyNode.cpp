#include "RigidBodyNode.h"
#include "ShapeNode.h"

RigidBodyNode::RigidBodyNode() {
    SetBodyType(PhysicsBodyType::Dynamic);
}

static void CollectShapesFrom(const Node* InNode, const Node3D& InBody, Array<PhysicsShapeDesc>& OutShapes) {
    const Quat inverseRotation = glm::inverse(InBody.GetRotation());
    const Vec3 bodyPosition = InBody.GetPosition();

    for (uint32_t i = 0; i < InNode->GetChildCount(); i++) {
        Node* child = InNode->GetChild((int32_t)i);
        if (Cast<RigidBodyNode>(child) != nullptr) {
            continue;
        }

        if (ShapeNode* shapeNode = Cast<ShapeNode>(child)) {
            PhysicsShapeDesc shape = shapeNode->GetShapeDesc();
            shape.Scale = shapeNode->GetScale();
            shape.LocalPosition = inverseRotation * (shapeNode->GetPosition() - bodyPosition);
            shape.LocalRotation = inverseRotation * shapeNode->GetRotation();
            OutShapes.Add(shape);
        }
        CollectShapesFrom(child, InBody, OutShapes);
    }
}

void RigidBodyNode::CollectShapes(Array<PhysicsShapeDesc>& OutShapes) const {
    CollectShapesFrom(this, *this, OutShapes);
}
