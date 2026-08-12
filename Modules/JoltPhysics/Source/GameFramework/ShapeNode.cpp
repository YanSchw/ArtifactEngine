#include "ShapeNode.h"
#include "RigidBodyNode.h"

RigidBodyNode* ShapeNode::GetOwningRigidBody() const {
    for (Node* parent = GetParent(); parent != nullptr; parent = parent->GetParent()) {
        if (RigidBodyNode* body = Cast<RigidBodyNode>(parent)) {
            return body;
        }
    }
    return nullptr;
}

void ShapeNode::CollectShapes(Array<PhysicsShapeDesc>& OutShapes) const {
    PhysicsShapeDesc shape = GetShapeDesc();
    shape.Scale = GetScale();
    OutShapes.Add(shape);
}

bool ShapeNode::ShouldCreateBody() const {
    return GetOwningRigidBody() == nullptr;
}

void ShapeNode::RefreshShape() {
    if (RigidBodyNode* owner = GetOwningRigidBody()) {
        owner->RefreshBody();
        return;
    }
    RefreshBody();
}
