#pragma once
#include "Node.h"
#include "Common/Types.h"
#include "Node3D.gen.h"

class World;
class Node3D;
class Component;

/** Node3D is a Node with a 3D Transform.
 *  The transform is stored as local position / rotation / scale
 *  World-space values are *derived* from the parent chain on demand. */
class Node3D : public Node {
public:
    ARTIFACT_CLASS();

    Node3D* GetTransform() override;

    Vec3 GetPosition() const;
    Vec3 GetLocalPosition() const;
    void SetPosition(const Vec3& InWorldPosition);
    void SetLocalPosition(const Vec3& InLocalPosition);
    void AddWorldOffset(const Vec3& InWorldOffset);
    void AddLocalOffset(const Vec3& InLocalOffset);

    Quat GetRotation() const;
    Quat GetLocalRotation() const;
    Vec3 GetEulerRotation() const;
    Vec3 GetLocalEulerRotation() const;
    void SetRotation(const Quat& InWorldRotation);
    void SetLocalRotation(const Quat& InLocalRotation);
    void SetEulerRotation(const Vec3& InWorldEulerAngles);
    void SetLocalEulerRotation(const Vec3& InLocalEulerAngles);
    void Rotate(const Quat& InDeltaRotation);
    void RotateEuler(const Vec3& InDeltaEulerAngles);
    void RotateWithAxis(const Vec3& InAxis, float InAngle);
    void SetLookDirection(const Vec3& InWorldDirection, const Vec3& InWorldUpDirection = VecUtils::Up);

    Vec3 GetScale() const;
    Vec3 GetLocalScale() const;
    void SetScale(const Vec3& InWorldScale);
    void SetLocalScale(const Vec3& InLocalScale);

    Mat4 GetTransformMatrix() const;
    Mat4 GetLocalTransformMatrix() const;
    void SetTransformMatrix(const Mat4& InWorldMatrix);

    static Mat4 CalculateTransformMatrix(const Vec3& InPosition, const Vec3& InEulerAngles, const Vec3& InScale);
    static Mat4 CalculateTransformMatrix(const Vec3& InPosition, const Quat& InRotation, const Vec3& InScale);

    Vec3 GetRightVector() const;
    Vec3 GetUpVector() const;
    Vec3 GetForwardVector() const;

protected:
    virtual void InitializeNode(World& OutWorld) override;

private:
    /** Nearest Node3D ancestor (skips non-spatial nodes); nullptr at the root. */
    Node3D* GetParentTransformChecked() const;

    Vec3 m_LocalPosition = Vec3(0.0f);
    Quat m_LocalRotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 m_LocalScale    = Vec3(1.0f);
};