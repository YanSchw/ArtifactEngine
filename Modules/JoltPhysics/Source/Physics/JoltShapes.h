#pragma once
#include "GameFramework/PhysicsTypes.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace JoltShapes {
    JPH::ShapeRefC Create(const PhysicsShapeDesc& InShape);
    JPH::ShapeRefC CreateCompound(const Array<PhysicsShapeDesc>& InShapes);
}
