#include "JoltShapes.h"
#include "JoltConversion.h"
#include "Core/Log.h"
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

namespace JoltShapes {

    static constexpr float MinExtent = 0.001f;

    static float MinComponent(const Vec3& InVector) {
        return glm::min(InVector.x, glm::min(InVector.y, InVector.z));
    }

    static float MaxComponent(const Vec3& InVector) {
        return glm::max(InVector.x, glm::max(InVector.y, InVector.z));
    }

    static JPH::ShapeRefC Build(const JPH::ShapeSettings& InSettings) {
        const JPH::ShapeSettings::ShapeResult result = InSettings.Create();
        if (result.HasError()) {
            AE_ERROR("Physics shape could not be created: {0}", result.GetError().c_str());
            return nullptr;
        }
        return result.Get();
    }

    static JPH::ShapeRefC CreatePrimitive(const PhysicsShapeDesc& InShape) {
        const Vec3 scale = glm::abs(InShape.Scale);
        const float radialScale = glm::max(scale.x, scale.z);
        const float halfHeight = glm::max(InShape.HalfHeight * scale.y, MinExtent);
        const float radius = glm::max(InShape.Radius * radialScale, MinExtent);

        switch (InShape.Type) {
            case PhysicsShapeType::Box: {
                const Vec3 halfExtents = glm::max(InShape.HalfExtents * scale, Vec3(MinExtent));
                const float convexRadius = glm::min(JPH::cDefaultConvexRadius, MinComponent(halfExtents) * 0.5f);
                return Build(JPH::BoxShapeSettings(ToJolt(halfExtents), convexRadius));
            }
            case PhysicsShapeType::Sphere:
                return Build(JPH::SphereShapeSettings(glm::max(InShape.Radius * MaxComponent(scale), MinExtent)));
            case PhysicsShapeType::Capsule:
                return Build(JPH::CapsuleShapeSettings(halfHeight, radius));
            case PhysicsShapeType::Cylinder:
                return Build(JPH::CylinderShapeSettings(halfHeight, radius));
        }
        return nullptr;
    }

    static bool HasLocalTransform(const PhysicsShapeDesc& InShape) {
        return InShape.LocalPosition != Vec3(0.0f) || InShape.LocalRotation != Quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    JPH::ShapeRefC Create(const PhysicsShapeDesc& InShape) {
        JPH::ShapeRefC shape = CreatePrimitive(InShape);
        if (shape == nullptr || !HasLocalTransform(InShape)) {
            return shape;
        }
        return Build(JPH::RotatedTranslatedShapeSettings(ToJolt(InShape.LocalPosition), ToJolt(InShape.LocalRotation), shape));
    }

    JPH::ShapeRefC CreateCompound(const Array<PhysicsShapeDesc>& InShapes) {
        Array<PhysicsShapeDesc> valid;
        Array<JPH::ShapeRefC> primitives;
        for (const PhysicsShapeDesc& shape : InShapes) {
            JPH::ShapeRefC primitive = CreatePrimitive(shape);
            if (primitive != nullptr) {
                valid.Add(shape);
                primitives.Add(primitive);
            }
        }

        if (primitives.IsEmpty()) {
            return nullptr;
        }
        if (primitives.Size() == 1) {
            return HasLocalTransform(valid[0])
                ? Build(JPH::RotatedTranslatedShapeSettings(ToJolt(valid[0].LocalPosition), ToJolt(valid[0].LocalRotation), primitives[0]))
                : primitives[0];
        }

        JPH::StaticCompoundShapeSettings compound;
        for (int32_t i = 0; i < primitives.Size(); i++) {
            compound.AddShape(ToJolt(valid[i].LocalPosition), ToJolt(valid[i].LocalRotation), primitives[i]);
        }
        return Build(compound);
    }

}
