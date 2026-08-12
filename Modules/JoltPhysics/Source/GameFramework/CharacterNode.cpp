#include "CharacterNode.h"
#include "PhysicsWorld.h"
#include "Physics/JoltConversion.h"
#include "Physics/JoltLayers.h"
#include "Physics/JoltSystem.h"
#include "Physics/PhysicsWorldImpl.h"
#include "GameFramework/World.h"
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

void CharacterNode::BeginPlay() {
    Super::BeginPlay();
    CreateCharacter();
}

void CharacterNode::EndPlay() {
    DestroyCharacter();
    Super::EndPlay();
}

PhysicsWorld* CharacterNode::GetPhysicsWorld() const {
    World* world = GetWorld();
    return world ? world->GetSubsystem<PhysicsWorld>() : nullptr;
}

float CharacterNode::GetCylinderHalfHeight() const {
    return glm::max(m_Height * 0.5f - m_Radius, 0.01f);
}

void CharacterNode::CreateCharacter() {
    PhysicsWorld* physics = GetPhysicsWorld();
    if (!physics || m_Character != nullptr) {
        return;
    }

    const float cylinderHalfHeight = GetCylinderHalfHeight();
    JPH::ShapeRefC shape = new JPH::CapsuleShape(cylinderHalfHeight, m_Radius);

    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mShape = shape;
    settings->mMass = m_Mass;
    settings->mMaxSlopeAngle = glm::radians(m_MaxSlopeAngle);
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), cylinderHalfHeight);
    settings->mInnerBodyShape = shape;
    settings->mInnerBodyLayer = PhysicsLayers::Moving;

    m_Character = new JPH::CharacterVirtual(settings, ToJolt(GetPosition()), ToJolt(GetRotation()), GetNodeId(), &physics->GetImpl().System);
    m_Character->AddRef();
    physics->RegisterCharacter(this);
}

void CharacterNode::DestroyCharacter() {
    if (m_Character == nullptr) {
        return;
    }

    if (PhysicsWorld* physics = GetPhysicsWorld()) {
        physics->UnregisterCharacter(this);
    }
    m_Character->Release();
    m_Character = nullptr;
}

void CharacterNode::RefreshCharacter() {
    if (m_Character == nullptr) {
        return;
    }
    DestroyCharacter();
    CreateCharacter();
}

void CharacterNode::StepCharacter(float InDeltaTime) {
    PhysicsWorld* physics = GetPhysicsWorld();
    if (m_Character == nullptr || !physics) {
        return;
    }

    if (glm::distance(FromJolt(m_Character->GetPosition()), GetPosition()) > 0.0001f) {
        m_Character->SetPosition(ToJolt(GetPosition()));
    }
    m_Character->SetRotation(ToJolt(GetRotation()));

    const Vec3 gravity = physics->GetGravity();
    Vec3 velocity = FromJolt(m_Character->GetLinearVelocity());
    if (IsGrounded() && velocity.y <= 0.0f) {
        velocity.y = 0.0f;
    } else {
        velocity += gravity * InDeltaTime;
    }
    m_Character->SetLinearVelocity(ToJolt(velocity));

    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
    updateSettings.mWalkStairsStepUp = JPH::Vec3(0.0f, m_StepHeight, 0.0f);

    JPH::PhysicsSystem& system = physics->GetImpl().System;
    m_Character->ExtendedUpdate(InDeltaTime, ToJolt(gravity), updateSettings,
        system.GetDefaultBroadPhaseLayerFilter(PhysicsLayers::Moving),
        system.GetDefaultLayerFilter(PhysicsLayers::Moving),
        {}, {}, JoltSystem::GetTempAllocator());

    SetPosition(FromJolt(m_Character->GetPosition()));
}

void CharacterNode::Move(const Vec3& InVelocity) {
    if (m_Character == nullptr) {
        return;
    }
    const Vec3 velocity = FromJolt(m_Character->GetLinearVelocity());
    m_Character->SetLinearVelocity(ToJolt(Vec3(InVelocity.x, velocity.y, InVelocity.z)));
}

void CharacterNode::Jump(float InJumpSpeed) {
    if (m_Character == nullptr || !IsGrounded()) {
        return;
    }
    const Vec3 velocity = FromJolt(m_Character->GetLinearVelocity());
    m_Character->SetLinearVelocity(ToJolt(Vec3(velocity.x, InJumpSpeed, velocity.z)));
}

Vec3 CharacterNode::GetVelocity() const {
    return m_Character ? FromJolt(m_Character->GetLinearVelocity()) : Vec3(0.0f);
}

void CharacterNode::SetVelocity(const Vec3& InVelocity) {
    if (m_Character != nullptr) {
        m_Character->SetLinearVelocity(ToJolt(InVelocity));
    }
}

bool CharacterNode::IsGrounded() const {
    return m_Character != nullptr && m_Character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
}

bool CharacterNode::IsOnSteepGround() const {
    return m_Character != nullptr && m_Character->GetGroundState() == JPH::CharacterBase::EGroundState::OnSteepGround;
}

Vec3 CharacterNode::GetGroundNormal() const {
    return m_Character ? FromJolt(m_Character->GetGroundNormal()) : Vec3(0.0f);
}

Node3D* CharacterNode::GetGroundNode() const {
    PhysicsWorld* physics = GetPhysicsWorld();
    if (m_Character == nullptr || !physics || m_Character->GetGroundBodyID().IsInvalid()) {
        return nullptr;
    }
    return physics->GetNodeFromBody(m_Character->GetGroundBodyID().GetIndexAndSequenceNumber());
}

void CharacterNode::SetRadius(float InRadius) {
    m_Radius = InRadius;
    RefreshCharacter();
}

void CharacterNode::SetHeight(float InHeight) {
    m_Height = InHeight;
    RefreshCharacter();
}

void CharacterNode::SetMaxSlopeAngle(float InDegrees) {
    m_MaxSlopeAngle = InDegrees;
    if (m_Character != nullptr) {
        m_Character->SetMaxSlopeAngle(glm::radians(m_MaxSlopeAngle));
    }
}

void CharacterNode::SetMass(float InMass) {
    m_Mass = InMass;
    if (m_Character != nullptr) {
        m_Character->SetMass(m_Mass);
    }
}
