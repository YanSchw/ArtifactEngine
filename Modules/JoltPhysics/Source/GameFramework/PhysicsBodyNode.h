#pragma once
#include "GameFramework/Node3D.h"
#include "PhysicsTypes.h"
#include "Common/Delegate.h"
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

    PROPERTY(Params = (Other))
    Delegate<PhysicsBodyNode*> OnCollisionEntered;

    PROPERTY(Params = (Other))
    Delegate<PhysicsBodyNode*> OnCollisionExited;

    PROPERTY(Params = (Other))
    Delegate<Node3D*> OnTriggerEntered;

    PROPERTY(Params = (Other))
    Delegate<Node3D*> OnTriggerExited;

    FUNCTION(Pure)
    PhysicsBodyType GetBodyType() const { return m_BodyType; }
    FUNCTION()
    void SetBodyType(PhysicsBodyType InBodyType);

    FUNCTION(Pure)
    bool IsTrigger() const { return m_IsTrigger; }
    FUNCTION()
    void SetTrigger(bool InIsTrigger);

    FUNCTION(Pure)
    float GetMass() const { return m_Mass; }
    FUNCTION()
    void SetMass(float InMass);

    FUNCTION(Pure)
    float GetFriction() const { return m_Friction; }
    FUNCTION()
    void SetFriction(float InFriction);

    FUNCTION(Pure)
    float GetRestitution() const { return m_Restitution; }
    FUNCTION()
    void SetRestitution(float InRestitution);

    FUNCTION(Pure)
    float GetLinearDamping() const { return m_LinearDamping; }
    FUNCTION()
    void SetLinearDamping(float InDamping);

    FUNCTION(Pure)
    float GetAngularDamping() const { return m_AngularDamping; }
    FUNCTION()
    void SetAngularDamping(float InDamping);

    FUNCTION(Pure)
    float GetGravityFactor() const { return m_GravityFactor; }
    FUNCTION()
    void SetGravityFactor(float InFactor);

    FUNCTION(Pure)
    Vec3 GetLinearVelocity() const;
    FUNCTION()
    void SetLinearVelocity(const Vec3& InVelocity);
    FUNCTION(Pure)
    Vec3 GetAngularVelocity() const;
    FUNCTION()
    void SetAngularVelocity(const Vec3& InVelocity);

    FUNCTION()
    void AddForce(const Vec3& InForce);
    FUNCTION()
    void AddForceAtPosition(const Vec3& InForce, const Vec3& InPosition);
    FUNCTION()
    void AddImpulse(const Vec3& InImpulse);
    FUNCTION()
    void AddImpulseAtPosition(const Vec3& InImpulse, const Vec3& InPosition);
    FUNCTION()
    void AddTorque(const Vec3& InTorque);
    FUNCTION()
    void AddAngularImpulse(const Vec3& InImpulse);

    FUNCTION(Pure)
    bool IsActive() const;
    FUNCTION()
    void Activate();
    FUNCTION()
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
