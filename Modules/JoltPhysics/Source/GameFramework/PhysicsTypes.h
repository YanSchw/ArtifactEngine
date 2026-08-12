#pragma once
#include "Common/Types.h"
#include "Common/Array.h"
#include "Object/Enum.h"
#include "PhysicsTypes.gen.h"

class Node;
class Node3D;

ARTIFACT_ENUM();
enum class PhysicsBodyType : uint8_t {
    Static = 0,
    Kinematic = 1,
    Dynamic = 2
};

ARTIFACT_ENUM();
enum class PhysicsShapeType : uint8_t {
    Box = 0,
    Sphere = 1,
    Capsule = 2,
    Cylinder = 3
};

struct PhysicsShapeDesc {
    PhysicsShapeType Type = PhysicsShapeType::Box;
    Vec3 HalfExtents = Vec3(0.5f);
    float Radius = 0.5f;
    float HalfHeight = 0.5f;
    Vec3 Scale = Vec3(1.0f);
    Vec3 LocalPosition = Vec3(0.0f);
    Quat LocalRotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
};

struct PhysicsHit {
    Node3D* Node = nullptr;
    Vec3 Position = Vec3(0.0f);
    Vec3 Normal = Vec3(0.0f);
    float Distance = 0.0f;
};

struct PhysicsQueryFilter {
    const Node* Ignore = nullptr;
    bool IncludeTriggers = false;
};
