#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace PhysicsLayers {
    static constexpr JPH::ObjectLayer NonMoving = 0;
    static constexpr JPH::ObjectLayer Moving = 1;
    static constexpr JPH::uint Count = 2;
}

namespace PhysicsBroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NonMoving(0);
    static constexpr JPH::BroadPhaseLayer Moving(1);
    static constexpr JPH::uint Count = 2;
}

class BroadPhaseLayerMap final : public JPH::BroadPhaseLayerInterface {
public:
    virtual JPH::uint GetNumBroadPhaseLayers() const override {
        return PhysicsBroadPhaseLayers::Count;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer InLayer) const override {
        return InLayer == PhysicsLayers::Moving ? PhysicsBroadPhaseLayers::Moving : PhysicsBroadPhaseLayers::NonMoving;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer InLayer) const override {
        return InLayer == PhysicsBroadPhaseLayers::Moving ? "Moving" : "NonMoving";
    }
#endif
};

class ObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer InLayer, JPH::BroadPhaseLayer InBroadPhaseLayer) const override {
        return InLayer == PhysicsLayers::Moving || InBroadPhaseLayer == PhysicsBroadPhaseLayers::Moving;
    }
};

class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer InFirst, JPH::ObjectLayer InSecond) const override {
        return InFirst == PhysicsLayers::Moving || InSecond == PhysicsLayers::Moving;
    }
};
