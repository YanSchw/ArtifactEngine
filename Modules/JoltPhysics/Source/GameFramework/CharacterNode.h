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
    void Move(const Vec3& InVelocity);
    void Jump(float InJumpSpeed);

    Vec3 GetVelocity() const;
    void SetVelocity(const Vec3& InVelocity);

    bool IsGrounded() const;
    bool IsOnSteepGround() const;
    Vec3 GetGroundNormal() const;
    Node3D* GetGroundNode() const;

    float GetRadius() const { return m_Radius; }
    void SetRadius(float InRadius);

    float GetHeight() const { return m_Height; }
    void SetHeight(float InHeight);

    float GetMaxSlopeAngle() const { return m_MaxSlopeAngle; }
    void SetMaxSlopeAngle(float InDegrees);

    float GetStepHeight() const { return m_StepHeight; }
    void SetStepHeight(float InStepHeight) { m_StepHeight = InStepHeight; }

    float GetMass() const { return m_Mass; }
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
