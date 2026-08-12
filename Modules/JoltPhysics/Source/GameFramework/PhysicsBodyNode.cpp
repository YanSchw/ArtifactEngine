#include "PhysicsBodyNode.h"
#include "PhysicsWorld.h"
#include "Physics/JoltConversion.h"
#include "Physics/JoltLayers.h"
#include "Physics/JoltShapes.h"
#include "Physics/PhysicsWorldImpl.h"
#include "Core/Log.h"
#include "GameFramework/World.h"
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>

static JPH::EMotionType ToJoltMotionType(PhysicsBodyType InBodyType) {
    switch (InBodyType) {
        case PhysicsBodyType::Kinematic: return JPH::EMotionType::Kinematic;
        case PhysicsBodyType::Dynamic: return JPH::EMotionType::Dynamic;
        case PhysicsBodyType::Static: break;
    }
    return JPH::EMotionType::Static;
}

void PhysicsBodyNode::BeginPlay() {
    Super::BeginPlay();
    if (ShouldCreateBody()) {
        CreateBody();
    }
}

void PhysicsBodyNode::EndPlay() {
    DestroyBody();
    Super::EndPlay();
}

PhysicsWorld* PhysicsBodyNode::GetPhysicsWorld() const {
    World* world = GetWorld();
    return world ? world->GetSubsystem<PhysicsWorld>() : nullptr;
}

JPH::BodyInterface* PhysicsBodyNode::GetBodyInterface() const {
    PhysicsWorld* physics = HasBody() ? GetPhysicsWorld() : nullptr;
    return physics ? &physics->GetImpl().System.GetBodyInterface() : nullptr;
}

void PhysicsBodyNode::CreateBody() {
    PhysicsWorld* physics = GetPhysicsWorld();
    if (!physics || HasBody()) {
        return;
    }

    Array<PhysicsShapeDesc> shapes;
    CollectShapes(shapes);

    JPH::ShapeRefC shape = JoltShapes::CreateCompound(shapes);
    if (shape == nullptr) {
        return;
    }

    const bool isStatic = m_BodyType == PhysicsBodyType::Static;
    JPH::BodyCreationSettings settings(shape, ToJolt(GetPosition()), ToJolt(GetRotation()), ToJoltMotionType(m_BodyType), isStatic ? PhysicsLayers::NonMoving : PhysicsLayers::Moving);
    settings.mIsSensor = m_IsTrigger;
    settings.mFriction = m_Friction;
    settings.mRestitution = m_Restitution;
    settings.mLinearDamping = m_LinearDamping;
    settings.mAngularDamping = m_AngularDamping;
    settings.mGravityFactor = m_GravityFactor;
    settings.mAllowSleeping = m_AllowSleeping;
    settings.mMotionQuality = m_UseContinuousCollision ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
    settings.mUserData = GetNodeId();
    if (m_Mass > 0.0f) {
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = m_Mass;
    }

    const JPH::BodyID bodyId = physics->GetImpl().System.GetBodyInterface().CreateAndAddBody(settings, isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
    if (bodyId.IsInvalid()) {
        AE_ERROR("'{0}' could not create a physics body", GetName());
        return;
    }

    m_BodyId = bodyId.GetIndexAndSequenceNumber();
    physics->RegisterBody(this);
}

void PhysicsBodyNode::DestroyBody() {
    if (!HasBody()) {
        return;
    }

    if (PhysicsWorld* physics = GetPhysicsWorld()) {
        JPH::BodyInterface& bodies = physics->GetImpl().System.GetBodyInterface();
        const JPH::BodyID bodyId(m_BodyId);
        bodies.RemoveBody(bodyId);
        bodies.DestroyBody(bodyId);
        physics->UnregisterBody(this);
    }
    m_BodyId = InvalidBodyId;
}

void PhysicsBodyNode::RefreshBody() {
    DestroyBody();
    if (WasBeginPlayCalled() && !IsPendingKill() && ShouldCreateBody()) {
        CreateBody();
    }
}

void PhysicsBodyNode::PushTransformToBody(float InDeltaTime) {
    JPH::BodyInterface* bodies = GetBodyInterface();
    if (!bodies) {
        return;
    }

    const JPH::BodyID bodyId(m_BodyId);
    const Vec3 position = GetPosition();
    const Quat rotation = GetRotation();
    const bool moved = glm::distance(FromJolt(bodies->GetPosition(bodyId)), position) > 0.0001f
        || glm::abs(glm::dot(FromJolt(bodies->GetRotation(bodyId)), rotation)) < 0.9999f;
    if (!moved) {
        return;
    }

    if (m_BodyType == PhysicsBodyType::Kinematic) {
        bodies->MoveKinematic(bodyId, ToJolt(position), ToJolt(rotation), InDeltaTime);
    } else {
        bodies->SetPositionAndRotation(bodyId, ToJolt(position), ToJolt(rotation), m_BodyType == PhysicsBodyType::Dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    }
}

void PhysicsBodyNode::PullTransformFromBody() {
    if (m_BodyType != PhysicsBodyType::Dynamic) {
        return;
    }

    JPH::BodyInterface* bodies = GetBodyInterface();
    const JPH::BodyID bodyId(m_BodyId);
    if (!bodies || !bodies->IsActive(bodyId)) {
        return;
    }

    SetPosition(FromJolt(bodies->GetPosition(bodyId)));
    SetRotation(FromJolt(bodies->GetRotation(bodyId)));
}

void PhysicsBodyNode::SetBodyType(PhysicsBodyType InBodyType) {
    if (m_BodyType == InBodyType) {
        return;
    }
    m_BodyType = InBodyType;
    RefreshBody();
}

void PhysicsBodyNode::SetTrigger(bool InIsTrigger) {
    if (m_IsTrigger == InIsTrigger) {
        return;
    }
    m_IsTrigger = InIsTrigger;
    RefreshBody();
}

void PhysicsBodyNode::SetMass(float InMass) {
    if (m_Mass == InMass) {
        return;
    }
    m_Mass = InMass;
    RefreshBody();
}

void PhysicsBodyNode::SetFriction(float InFriction) {
    m_Friction = InFriction;
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->SetFriction(JPH::BodyID(m_BodyId), InFriction);
    }
}

void PhysicsBodyNode::SetRestitution(float InRestitution) {
    m_Restitution = InRestitution;
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->SetRestitution(JPH::BodyID(m_BodyId), InRestitution);
    }
}

void PhysicsBodyNode::SetLinearDamping(float InDamping) {
    m_LinearDamping = InDamping;
    RefreshBody();
}

void PhysicsBodyNode::SetAngularDamping(float InDamping) {
    m_AngularDamping = InDamping;
    RefreshBody();
}

void PhysicsBodyNode::SetGravityFactor(float InFactor) {
    m_GravityFactor = InFactor;
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->SetGravityFactor(JPH::BodyID(m_BodyId), InFactor);
    }
}

Vec3 PhysicsBodyNode::GetLinearVelocity() const {
    JPH::BodyInterface* bodies = GetBodyInterface();
    return bodies ? FromJolt(bodies->GetLinearVelocity(JPH::BodyID(m_BodyId))) : Vec3(0.0f);
}

void PhysicsBodyNode::SetLinearVelocity(const Vec3& InVelocity) {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->SetLinearVelocity(JPH::BodyID(m_BodyId), ToJolt(InVelocity));
    }
}

Vec3 PhysicsBodyNode::GetAngularVelocity() const {
    JPH::BodyInterface* bodies = GetBodyInterface();
    return bodies ? FromJolt(bodies->GetAngularVelocity(JPH::BodyID(m_BodyId))) : Vec3(0.0f);
}

void PhysicsBodyNode::SetAngularVelocity(const Vec3& InVelocity) {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->SetAngularVelocity(JPH::BodyID(m_BodyId), ToJolt(InVelocity));
    }
}

void PhysicsBodyNode::AddForce(const Vec3& InForce) {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->AddForce(JPH::BodyID(m_BodyId), ToJolt(InForce));
    }
}

void PhysicsBodyNode::AddForceAtPosition(const Vec3& InForce, const Vec3& InPosition) {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->AddForce(JPH::BodyID(m_BodyId), ToJolt(InForce), ToJolt(InPosition));
    }
}

void PhysicsBodyNode::AddImpulse(const Vec3& InImpulse) {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->AddImpulse(JPH::BodyID(m_BodyId), ToJolt(InImpulse));
    }
}

void PhysicsBodyNode::AddImpulseAtPosition(const Vec3& InImpulse, const Vec3& InPosition) {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->AddImpulse(JPH::BodyID(m_BodyId), ToJolt(InImpulse), ToJolt(InPosition));
    }
}

void PhysicsBodyNode::AddTorque(const Vec3& InTorque) {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->AddTorque(JPH::BodyID(m_BodyId), ToJolt(InTorque));
    }
}

void PhysicsBodyNode::AddAngularImpulse(const Vec3& InImpulse) {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->AddAngularImpulse(JPH::BodyID(m_BodyId), ToJolt(InImpulse));
    }
}

bool PhysicsBodyNode::IsActive() const {
    JPH::BodyInterface* bodies = GetBodyInterface();
    return bodies && bodies->IsActive(JPH::BodyID(m_BodyId));
}

void PhysicsBodyNode::Activate() {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->ActivateBody(JPH::BodyID(m_BodyId));
    }
}

void PhysicsBodyNode::Deactivate() {
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->DeactivateBody(JPH::BodyID(m_BodyId));
    }
}

void PhysicsBodyNode::SetAllowSleeping(bool InAllowSleeping) {
    m_AllowSleeping = InAllowSleeping;

    PhysicsWorld* physics = HasBody() ? GetPhysicsWorld() : nullptr;
    if (!physics) {
        return;
    }

    const JPH::BodyLockWrite lock(physics->GetImpl().System.GetBodyLockInterface(), JPH::BodyID(m_BodyId));
    if (lock.Succeeded()) {
        lock.GetBody().SetAllowSleeping(InAllowSleeping);
    }
}

void PhysicsBodyNode::SetUseContinuousCollision(bool InUseContinuousCollision) {
    m_UseContinuousCollision = InUseContinuousCollision;
    if (JPH::BodyInterface* bodies = GetBodyInterface()) {
        bodies->SetMotionQuality(JPH::BodyID(m_BodyId), InUseContinuousCollision ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete);
    }
}
