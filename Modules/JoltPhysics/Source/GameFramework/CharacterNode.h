#pragma once
#include "GameFramework/Node3D.h"
#include "PhysicsTypes.h"
#include "CharacterNode.gen.h"

class PhysicsWorld;

namespace JPH {
    class CharacterVirtual;
}

/** A capsule that walks the world: it is moved by velocity and slides along geometry instead of
 *  being simulated as a rigid body. */
class CharacterNode : public Node3D {
public:
    ARTIFACT_CLASS();

    virtual void BeginPlay() override;
    virtual void EndPlay() override;

    /** Horizontal movement for this frame; the vertical speed stays under the character's control. */
    FUNCTION()
    void Move(const Vec3& InVelocity);
    FUNCTION()
    void Jump(float InJumpSpeed);

    FUNCTION(Pure)
    Vec3 GetVelocity() const;
    FUNCTION()
    void SetVelocity(const Vec3& InVelocity);

    FUNCTION(Pure)
    bool IsGrounded() const;
    FUNCTION(Pure)
    bool IsOnSteepGround() const;
    FUNCTION(Pure)
    Vec3 GetGroundNormal() const;
    FUNCTION(Pure)
    Node3D* GetGroundNode() const;

    FUNCTION(Pure)
    float GetRadius() const { return m_Radius; }
    FUNCTION()
    void SetRadius(float InRadius);

    FUNCTION(Pure)
    float GetHeight() const { return m_Height; }
    FUNCTION()
    void SetHeight(float InHeight);

    FUNCTION(Pure)
    float GetMaxSlopeAngle() const { return m_MaxSlopeAngle; }
    FUNCTION()
    void SetMaxSlopeAngle(float InDegrees);

    FUNCTION(Pure)
    float GetStepHeight() const { return m_StepHeight; }
    FUNCTION()
    void SetStepHeight(float InStepHeight) { m_StepHeight = InStepHeight; }

    FUNCTION(Pure)
    float GetMass() const { return m_Mass; }
    FUNCTION()
    void SetMass(float InMass);

private:
    void CreateCharacter();
    void DestroyCharacter();
    void StepCharacter(float InDeltaTime);
    void RefreshCharacter();

    PhysicsWorld* GetPhysicsWorld() const;
    float GetCylinderHalfHeight() const;

    JPH::CharacterVirtual* m_Character = nullptr;

    PROPERTY()
    float m_Radius = 0.3f;

    PROPERTY()
    float m_Height = 1.8f;

    PROPERTY()
    float m_MaxSlopeAngle = 45.0f;

    PROPERTY()
    float m_StepHeight = 0.3f;

    PROPERTY()
    float m_Mass = 70.0f;

    friend class PhysicsWorld;
};
