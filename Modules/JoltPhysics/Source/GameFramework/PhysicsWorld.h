#pragma once
#include "GameFramework/WorldSubsystem.h"
#include "PhysicsTypes.h"
#include "PhysicsWorld.gen.h"

class CharacterNode;
class PhysicsBodyNode;
struct PhysicsWorldImpl;

/** The simulation every physics Node of one World lives in: it owns the Jolt scene, steps it at a
 *  fixed rate, keeps the Nodes and their bodies in sync and answers the scene queries. */
class PhysicsWorld : public WorldSubsystem {
public:
    ARTIFACT_CLASS();

    virtual ~PhysicsWorld();

    virtual void Initialize() override;
    virtual void WorldUpdate(float InDeltaTime) override;
    virtual void Shutdown() override;

    Vec3 GetGravity() const;
    void SetGravity(const Vec3& InGravity);

    float GetFixedTimeStep() const { return m_FixedTimeStep; }
    void SetFixedTimeStep(float InTimeStep);

    bool RayCast(const Vec3& InOrigin, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter = {}) const;
    Array<PhysicsHit> RayCastAll(const Vec3& InOrigin, const Vec3& InDirection, float InMaxDistance, const PhysicsQueryFilter& InFilter = {}) const;

    bool ShapeCast(const PhysicsShapeDesc& InShape, const Vec3& InOrigin, const Quat& InRotation, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter = {}) const;
    bool SphereCast(float InRadius, const Vec3& InOrigin, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter = {}) const;
    bool BoxCast(const Vec3& InHalfExtents, const Vec3& InOrigin, const Quat& InRotation, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter = {}) const;
    bool CapsuleCast(float InRadius, float InHalfHeight, const Vec3& InOrigin, const Quat& InRotation, const Vec3& InDirection, float InMaxDistance, PhysicsHit& OutHit, const PhysicsQueryFilter& InFilter = {}) const;

    Array<Node3D*> OverlapShape(const PhysicsShapeDesc& InShape, const Vec3& InCenter, const Quat& InRotation, const PhysicsQueryFilter& InFilter = {}) const;
    Array<Node3D*> OverlapSphere(float InRadius, const Vec3& InCenter, const PhysicsQueryFilter& InFilter = {}) const;
    Array<Node3D*> OverlapBox(const Vec3& InHalfExtents, const Vec3& InCenter, const Quat& InRotation, const PhysicsQueryFilter& InFilter = {}) const;

    Node3D* GetNodeFromBody(uint32_t InBodyId) const;

private:
    PhysicsWorldImpl& GetImpl() const { return *m_Impl; }

    void RegisterBody(PhysicsBodyNode* InBody);
    void UnregisterBody(PhysicsBodyNode* InBody);
    void RegisterCharacter(CharacterNode* InCharacter);
    void UnregisterCharacter(CharacterNode* InCharacter);

    void Step(float InDeltaTime);

    PhysicsWorldImpl* m_Impl = nullptr;
    Array<PhysicsBodyNode*> m_Bodies;
    Array<CharacterNode*> m_Characters;

    Vec3 m_Gravity = Vec3(0.0f, -9.81f, 0.0f);
    float m_FixedTimeStep = 1.0f / 60.0f;
    float m_Accumulator = 0.0f;
    int32_t m_MaxStepsPerUpdate = 4;
    bool m_BroadPhaseDirty = false;

    friend class PhysicsBodyNode;
    friend class CharacterNode;
};
