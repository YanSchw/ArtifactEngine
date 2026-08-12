#include "PhysicsWorld.h"
#include "CharacterNode.h"
#include "PhysicsBodyNode.h"
#include "Physics/JoltConversion.h"
#include "Physics/JoltShapes.h"
#include "Physics/JoltSystem.h"
#include "Physics/PhysicsWorldImpl.h"
#include "GameFramework/Node3D.h"
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

static constexpr uint32_t MaxBodies = 16384;
static constexpr uint32_t MaxBodyPairs = 16384;
static constexpr uint32_t MaxContactConstraints = 8192;
static constexpr float MaxAccumulatedTime = 0.25f;

namespace {

    class QueryBodyFilter final : public JPH::BodyFilter {
    public:
        explicit QueryBodyFilter(const PhysicsQueryFilter& InFilter)
            : m_IgnoreNodeId(InFilter.Ignore ? InFilter.Ignore->GetNodeId() : 0)
            , m_IncludeTriggers(InFilter.IncludeTriggers) {}

        virtual bool ShouldCollideLocked(const JPH::Body& InBody) const override {
            if (!m_IncludeTriggers && InBody.IsSensor()) {
                return false;
            }
            return m_IgnoreNodeId == 0 || InBody.GetUserData() != m_IgnoreNodeId;
        }

    private:
        uint64_t m_IgnoreNodeId;
        bool m_IncludeTriggers;
    };

}

void PhysicsWorld::Initialize() {
    JoltSystem::Acquire();

    m_Impl = new PhysicsWorldImpl();
    m_Impl->System.Init(MaxBodies, 0, MaxBodyPairs, MaxContactConstraints, m_Impl->BroadPhaseLayers, m_Impl->BroadPhaseFilter, m_Impl->LayerFilter);
    m_Impl->System.SetContactListener(&m_Impl->ContactListener);
    m_Impl->System.SetGravity(ToJolt(m_Gravity));
}

PhysicsWorld::~PhysicsWorld() {
    Shutdown();
}

void PhysicsWorld::Shutdown() {
    if (m_Impl == nullptr) {
        return;
    }

    m_Bodies.Clear();
    m_Characters.Clear();

    delete m_Impl;
    m_Impl = nullptr;

    JoltSystem::Release();
}

void PhysicsWorld::WorldUpdate(float InDeltaTime) {
    if (m_BroadPhaseDirty) {
        m_Impl->System.OptimizeBroadPhase();
        m_BroadPhaseDirty = false;
    }

    m_Accumulator = glm::min(m_Accumulator + InDeltaTime, MaxAccumulatedTime);
    for (int32_t step = 0; step < m_MaxStepsPerUpdate && m_Accumulator >= m_FixedTimeStep; step++) {
        m_Accumulator -= m_FixedTimeStep;
        Step(m_FixedTimeStep);
    }
}

void PhysicsWorld::Step(float InDeltaTime) {
    for (int32_t i = m_Bodies.Last(); i >= 0; i--) {
        m_Bodies[i]->PushTransformToBody(InDeltaTime);
    }
    for (int32_t i = m_Characters.Last(); i >= 0; i--) {
        m_Characters[i]->StepCharacter(InDeltaTime);
    }

    m_Impl->System.Update(InDeltaTime, 1, &JoltSystem::GetTempAllocator(), &JoltSystem::GetJobSystem());

    for (int32_t i = m_Bodies.Last(); i >= 0; i--) {
        m_Bodies[i]->PullTransformFromBody();
    }
    m_Impl->ContactListener.Dispatch(*this);
}

Vec3 PhysicsWorld::GetGravity() const {
    return m_Gravity;
}

void PhysicsWorld::SetGravity(const Vec3& InGravity) {
    m_Gravity = InGravity;
    if (m_Impl) {
        m_Impl->System.SetGravity(ToJolt(m_Gravity));
    }
}

void PhysicsWorld::SetFixedTimeStep(float InTimeStep) {
    m_FixedTimeStep = glm::max(InTimeStep, 0.001f);
}

Node3D* PhysicsWorld::GetNodeFromBody(uint32_t InBodyId) const {
    const uint64_t nodeId = m_Impl->System.GetBodyInterface().GetUserData(JPH::BodyID(InBodyId));
    return nodeId != 0 ? Cast<Node3D>(Node::FindById((uint32_t)nodeId)) : nullptr;
}

void PhysicsWorld::RegisterBody(PhysicsBodyNode* InBody) {
    m_Bodies.Add(InBody);
    m_BroadPhaseDirty = true;
}

void PhysicsWorld::UnregisterBody(PhysicsBodyNode* InBody) {
    m_Bodies.Remove(InBody);
}

void PhysicsWorld::RegisterCharacter(CharacterNode* InCharacter) {
    m_Characters.Add(InCharacter);
}

void PhysicsWorld::UnregisterCharacter(CharacterNode* InCharacter) {
    m_Characters.Remove(InCharacter);
}

static PhysicsHit MakeRayHit(const PhysicsWorld& InWorld, const JPH::PhysicsSystem& InSystem, const JPH::RRayCast& InRay, const JPH::RayCastResult& InResult, float InMaxDistance) {
    PhysicsHit hit;
    hit.Node = InWorld.GetNodeFromBody(InResult.mBodyID.GetIndexAndSequenceNumber());
    hit.Position = FromJolt(InRay.GetPointOnRay(InResult.mFraction));
    hit.Distance = InResult.mFraction * InMaxDistance;

    const JPH::BodyLockRead lock(InSystem.GetBodyLockInterface(), InResult.mBodyID);
    if (lock.Succeeded()) {
        hit.Normal = FromJolt(lock.GetBody().GetWorldSpaceSurfaceNormal(InResult.mSubShapeID2, ToJolt(hit.Position)));
    }
    return hit;
}

bool PhysicsWorld::RayCast(const Vec3& InOrigin, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter) const {
    const JPH::RRayCast ray(ToJolt(InOrigin), ToJolt(glm::normalize(InDirection) * InMaxDistance));
    const QueryBodyFilter bodyFilter(InFilter);

    JPH::RayCastResult result;
    if (!m_Impl->System.GetNarrowPhaseQuery().CastRay(ray, result, {}, {}, bodyFilter)) {
        return false;
    }

    OutHit = MakeRayHit(*this, m_Impl->System, ray, result, InMaxDistance);
    return true;
}

Array<PhysicsHit> PhysicsWorld::RayCastAll(const Vec3& InOrigin, const Vec3& InDirection, float InMaxDistance, const PhysicsQueryFilter& InFilter) const {
    const JPH::RRayCast ray(ToJolt(InOrigin), ToJolt(glm::normalize(InDirection) * InMaxDistance));
    const QueryBodyFilter bodyFilter(InFilter);

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    m_Impl->System.GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings(), collector, {}, {}, bodyFilter);
    collector.Sort();

    Array<PhysicsHit> hits;
    for (const JPH::RayCastResult& result : collector.mHits) {
        hits.Add(MakeRayHit(*this, m_Impl->System, ray, result, InMaxDistance));
    }
    return hits;
}

bool PhysicsWorld::ShapeCast(const PhysicsShapeDesc& InShape, const Vec3& InOrigin, const Quat& InRotation, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter) const {
    JPH::ShapeRefC shape = JoltShapes::Create(InShape);
    if (shape == nullptr) {
        return false;
    }

    const JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(ToJolt(InRotation), ToJolt(InOrigin));
    const JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(shape, JPH::Vec3::sReplicate(1.0f), transform, ToJolt(glm::normalize(InDirection) * InMaxDistance));
    const QueryBodyFilter bodyFilter(InFilter);

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_Impl->System.GetNarrowPhaseQuery().CastShape(shapeCast, JPH::ShapeCastSettings(), JPH::RVec3::sZero(), collector, {}, {}, bodyFilter);
    if (!collector.HadHit()) {
        return false;
    }

    OutHit.Node = GetNodeFromBody(collector.mHit.mBodyID2.GetIndexAndSequenceNumber());
    OutHit.Position = FromJolt(collector.mHit.mContactPointOn2);
    OutHit.Normal = FromJolt(-collector.mHit.mPenetrationAxis.Normalized());
    OutHit.Distance = collector.mHit.mFraction * InMaxDistance;
    return true;
}

bool PhysicsWorld::SphereCast(float InRadius, const Vec3& InOrigin, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter) const {
    PhysicsShapeDesc shape;
    shape.Type = PhysicsShapeType::Sphere;
    shape.Radius = InRadius;
    return ShapeCast(shape, InOrigin, Quat(1.0f, 0.0f, 0.0f, 0.0f), InDirection, InMaxDistance, OutHit, InFilter);
}

bool PhysicsWorld::BoxCast(const Vec3& InHalfExtents, const Vec3& InOrigin, const Quat& InRotation, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter) const {
    PhysicsShapeDesc shape;
    shape.Type = PhysicsShapeType::Box;
    shape.HalfExtents = InHalfExtents;
    return ShapeCast(shape, InOrigin, InRotation, InDirection, InMaxDistance, OutHit, InFilter);
}

bool PhysicsWorld::CapsuleCast(float InRadius, float InHalfHeight, const Vec3& InOrigin, const Quat& InRotation, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter) const {
    PhysicsShapeDesc shape;
    shape.Type = PhysicsShapeType::Capsule;
    shape.Radius = InRadius;
    shape.HalfHeight = InHalfHeight;
    return ShapeCast(shape, InOrigin, InRotation, InDirection, InMaxDistance, OutHit, InFilter);
}

Array<Node3D*> PhysicsWorld::OverlapShape(const PhysicsShapeDesc& InShape, const Vec3& InCenter, const Quat& InRotation, const PhysicsQueryFilter& InFilter) const {
    Array<Node3D*> nodes;

    JPH::ShapeRefC shape = JoltShapes::Create(InShape);
    if (shape == nullptr) {
        return nodes;
    }

    const JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(ToJolt(InRotation), ToJolt(InCenter)).PreTranslated(-shape->GetCenterOfMass());
    const QueryBodyFilter bodyFilter(InFilter);

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    m_Impl->System.GetNarrowPhaseQuery().CollideShape(shape, JPH::Vec3::sReplicate(1.0f), transform, JPH::CollideShapeSettings(), JPH::RVec3::sZero(), collector, {}, {}, bodyFilter);

    for (const JPH::CollideShapeResult& result : collector.mHits) {
        Node3D* node = GetNodeFromBody(result.mBodyID2.GetIndexAndSequenceNumber());
        if (node && !nodes.Contains(node)) {
            nodes.Add(node);
        }
    }
    return nodes;
}

Array<Node3D*> PhysicsWorld::OverlapSphere(float InRadius, const Vec3& InCenter, const PhysicsQueryFilter& InFilter) const {
    PhysicsShapeDesc shape;
    shape.Type = PhysicsShapeType::Sphere;
    shape.Radius = InRadius;
    return OverlapShape(shape, InCenter, Quat(1.0f, 0.0f, 0.0f, 0.0f), InFilter);
}

Array<Node3D*> PhysicsWorld::OverlapBox(const Vec3& InHalfExtents, const Vec3& InCenter, const Quat& InRotation, const PhysicsQueryFilter& InFilter) const {
    PhysicsShapeDesc shape;
    shape.Type = PhysicsShapeType::Box;
    shape.HalfExtents = InHalfExtents;
    return OverlapShape(shape, InCenter, InRotation, InFilter);
}
