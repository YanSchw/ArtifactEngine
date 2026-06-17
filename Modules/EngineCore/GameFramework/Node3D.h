#pragma once
#include "Node.h"
#include "Common/Types.h"
#include "Node3D.gen.h"

class World;
class Node3D;
class Component;

/** Node3D is a Node with a 3D Transform */
class Node3D : public Node {
public:
    ARTIFACT_CLASS();

    Node3D* GetTransform() override;

    Vec3 GetPosition() const;
    Vec3 GetLocalPosition() const;
    void SetPosition(const Vec3& InPosition);
    void SetLocalPosition(const Vec3& InPosition);
    void AddWorldOffset(const Vec3& InOffset);
    void AddLocalOffset(const Vec3& InOffset);

    Quat GetRotation() const;
    Quat GetLocalRotation() const;
    Vec3 GetEulerRotation() const;
    Vec3 GetLocalEulerRotation() const;
    void SetRotation(const Quat& InRotation);
    void SetLocalRotation(const Quat& InRotation);
    void SetEulerRotation(const Vec3& InEulerAngles);
    void SetLocalEulerRotation(const Vec3& InEulerAngles);
    void Rotate(const Quat& InQuat);
    void RotateEuler(const Vec3& InEulerAngles);
    void RotateWithAxis(const Vec3& InAxis, float InAngle);
    void SetLookDirection(const Vec3& InDirection, const Vec3& InUpDirection = VecUtils::Up);

    Vec3 GetScale() const;
    Vec3 GetLocalScale() const;
    void SetScale(const Vec3& InScale);
    void SetLocalScale(const Vec3& InScale);

    Mat4 GetTransformMatrix() const;
    void SetTransformMatrix(const Mat4& mat);
    void RecalculateTransformMatrix();
    void ReprojectLocalMatrixToWorld();

    static Mat4 CalculateTransformMatrix(const Vec3& InPosition, const Vec3& InEulerAngles, const Vec3& InScale);
    static Mat4 CalculateTransformMatrix(const Vec3& InPosition, const Quat& InRotation, const Vec3& InScale);

    Vec3 GetRightVector() const;
    Vec3 GetUpVector() const;
    Vec3 GetForwardVector() const;

protected:
    void TickTransform(bool InInWorldSpace = false) override;
    virtual void InitializeNode(World& OutWorld) override;

private:
    Mat4 m_WorldTransformMatrix = Mat4(1);
    Mat4 m_LocalTransformMatrix = Mat4(1);
};