#pragma once
#include "GameFramework/Node3D.h"
#include "PhysicsTypes.h"
#include "PhysicsBodyNode.gen.h"

class PhysicsWorld;

namespace JPH {
    class BodyInterface;
}

class PhysicsBodyNode : public Node3D {
public:
    ARTIFACT_CLASS();

    virtual void BeginPlay() override;
    virtual void EndPlay() override;

    virtual void OnCollisionEnter(PhysicsBodyNode* InOther) { (void)InOther; }
    virtual void OnCollisionExit(PhysicsBodyNode* InOther) { (void)InOther; }
    virtual void OnTriggerEnter(Node3D* InOther) { (void)InOther; }
    virtual void OnTriggerExit(Node3D* InOther) { (void)InOther; }

    PhysicsBodyType GetBodyType() const { return m_BodyType; }
    void SetBodyType(PhysicsBodyType InBodyType);

    bool IsTrigger() const { return m_IsTrigger; }
    void SetTrigger(bool InIsTrigger);

    float GetMass() const { return m_Mass; }
    void SetMass(float InMass);

    float GetFriction() const { return m_Friction; }
    void SetFriction(float InFriction);

    float GetRestitution() const { return m_Restitution; }
    void SetRestitution(float InRestitution);

    float GetLinearDamping() const { return m_LinearDamping; }
    void SetLinearDamping(float InDamping);

    float GetAngularDamping() const { return m_AngularDamping; }
    void SetAngularDamping(float InDamping);

    float GetGravityFactor() const { return m_GravityFactor; }
    void SetGravityFactor(float InFactor);

    Vec3 GetLinearVelocity() const;
    void SetLinearVelocity(const Vec3& InVelocity);
    Vec3 GetAngularVelocity() const;
    void SetAngularVelocity(const Vec3& InVelocity);

    void AddForce(const Vec3& InForce);
    void AddForceAtPosition(const Vec3& InForce, const Vec3& InPosition);
    void AddImpulse(const Vec3& InImpulse);
    void AddImpulseAtPosition(const Vec3& InImpulse, const Vec3& InPosition);
    void AddTorque(const Vec3& InTorque);
    void AddAngularImpulse(const Vec3& InImpulse);

    bool IsActive() const;
    void Activate();
    void Deactivate();

    bool GetAllowSleeping() const { return m_AllowSleeping; }
    void SetAllowSleeping(bool InAllowSleeping);

    bool GetUseContinuousCollision() const { return m_UseContinuousCollision; }
    void SetUseContinuousCollision(bool InUseContinuousCollision);

    bool HasBody() const { return m_BodyId != InvalidBodyId; }
    uint32_t GetBodyId() const { return m_BodyId; }

    void RefreshBody();

protected:
    virtual void CollectShapes(Array<PhysicsShapeDesc>& OutShapes) const = 0;
    virtual bool ShouldCreateBody() const { return true; }

    PhysicsWorld* GetPhysicsWorld() const;

private:
    static constexpr uint32_t InvalidBodyId = 0xffffffff;

    void CreateBody();
    void DestroyBody();
    void PushTransformToBody(float InDeltaTime);
    void PullTransformFromBody();

    JPH::BodyInterface* GetBodyInterface() const;

    uint32_t m_BodyId = InvalidBodyId;

    PROPERTY()
    PhysicsBodyType m_BodyType = PhysicsBodyType::Static;

    PROPERTY()
    bool m_IsTrigger = false;

    PROPERTY()
    float m_Mass = 0.0f;

    PROPERTY()
    float m_Friction = 0.2f;

    PROPERTY()
    float m_Restitution = 0.0f;

    PROPERTY()
    float m_LinearDamping = 0.05f;

    PROPERTY()
    float m_AngularDamping = 0.05f;

    PROPERTY()
    float m_GravityFactor = 1.0f;

    PROPERTY()
    bool m_AllowSleeping = true;

    PROPERTY()
    bool m_UseContinuousCollision = false;

    friend class PhysicsWorld;
};
