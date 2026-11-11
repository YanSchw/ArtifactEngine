#pragma once
#include "Node.h"
#include "Common/Types.h"
#include "Node3D.gen.h"

class World;
class Node3D;
class Component;

struct Node3DSpecialProperties { Node3DSpecialProperties(); };

/** Node3D is a Node with a 3D Transform.
 *  The transform is stored as local position / rotation / scale
 *  World-space values are *derived* from the parent chain on demand. */
class Node3D : public Node {
public:
    ARTIFACT_CLASS();

    Node3D* GetTransform() override;

    FUNCTION(Pure)
    Vec3 GetPosition() const;
    FUNCTION(Pure)
    Vec3 GetLocalPosition() const;
    FUNCTION()
    void SetPosition(const Vec3& InWorldPosition);
    FUNCTION()
    void SetLocalPosition(const Vec3& InLocalPosition);
    FUNCTION()
    void AddWorldOffset(const Vec3& InWorldOffset);
    FUNCTION()
    void AddLocalOffset(const Vec3& InLocalOffset);

    FUNCTION(Pure)
    Quat GetRotation() const;
    FUNCTION(Pure)
    Quat GetLocalRotation() const;
    FUNCTION(Pure)
    Vec3 GetEulerRotation() const;
    FUNCTION(Pure)
    Vec3 GetLocalEulerRotation() const;
    FUNCTION()
    void SetRotation(const Quat& InWorldRotation);
    FUNCTION()
    void SetLocalRotation(const Quat& InLocalRotation);
    FUNCTION()
    void SetEulerRotation(const Vec3& InWorldEulerAngles);
    FUNCTION()
    void SetLocalEulerRotation(const Vec3& InLocalEulerAngles);
    FUNCTION()
    void Rotate(const Quat& InDeltaRotation);
    FUNCTION()
    void RotateEuler(const Vec3& InDeltaEulerAngles);
    FUNCTION()
    void RotateWithAxis(const Vec3& InAxis, float InAngle);
    FUNCTION()
    void SetLookDirection(const Vec3& InWorldDirection, const Vec3& InWorldUpDirection = VecUtils::Up);

    FUNCTION(Pure)
    Vec3 GetScale() const;
    FUNCTION(Pure)
    Vec3 GetLocalScale() const;
    FUNCTION()
    void SetScale(const Vec3& InWorldScale);
    FUNCTION()
    void SetLocalScale(const Vec3& InLocalScale);

    Mat4 GetTransformMatrix() const;
    Mat4 GetLocalTransformMatrix() const;
    void SetTransformMatrix(const Mat4& InWorldMatrix);

    static Mat4 CalculateTransformMatrix(const Vec3& InPosition, const Vec3& InEulerAngles, const Vec3& InScale);
    static Mat4 CalculateTransformMatrix(const Vec3& InPosition, const Quat& InRotation, const Vec3& InScale);

    FUNCTION(Pure)
    Vec3 GetRightVector() const;
    FUNCTION(Pure)
    Vec3 GetUpVector() const;
    FUNCTION(Pure)
    Vec3 GetForwardVector() const;

protected:
    virtual void InitializeNode(World& OutWorld) override;

private:
    /** Nearest Node3D ancestor (skips non-spatial nodes); nullptr at the root. */
    Node3D* GetParentTransformChecked() const;

    Vec3 m_LocalPosition = Vec3(0.0f);
    Quat m_LocalRotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 m_LocalScale    = Vec3(1.0f);

    inline static Node3DSpecialProperties s_SpecialProperties;

    friend struct Node3DSpecialProperties;
};
