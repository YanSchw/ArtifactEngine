#include "Node3D.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

Node3D* Node3D::GetTransform() {
    return this;
}

Node3D* Node3D::GetParentTransformChecked() const {
    // GetParentTransform() only reads the hierarchy; it is non-const purely by convention.
    return const_cast<Node3D*>(this)->GetParentTransform();
}

/* ---- Position ---- */

Vec3 Node3D::GetPosition() const {
    return Vec3(GetTransformMatrix()[3]);
}

Vec3 Node3D::GetLocalPosition() const {
    return m_LocalPosition;
}

void Node3D::SetPosition(const Vec3& InWorldPosition) {
    if (Node3D* parent = GetParentTransform()) {
        m_LocalPosition = Vec3(glm::inverse(parent->GetTransformMatrix()) * Vec4(InWorldPosition, 1.0f));
    } else {
        m_LocalPosition = InWorldPosition;
    }
    TickTransform();
}

void Node3D::SetLocalPosition(const Vec3& InLocalPosition) {
    m_LocalPosition = InLocalPosition;
    TickTransform();
}

void Node3D::AddWorldOffset(const Vec3& InWorldOffset) {
    SetPosition(GetPosition() + InWorldOffset);
}

void Node3D::AddLocalOffset(const Vec3& InLocalOffset) {
    m_LocalPosition += InLocalOffset;
    TickTransform();
}

/* ---- Rotation ---- */

Quat Node3D::GetRotation() const {
    Node3D* parent = GetParentTransformChecked();
    return parent ? glm::normalize(parent->GetRotation() * m_LocalRotation) : m_LocalRotation;
}

Quat Node3D::GetLocalRotation() const {
    return m_LocalRotation;
}

Vec3 Node3D::GetEulerRotation() const {
    return glm::degrees(glm::eulerAngles(GetRotation()));
}

Vec3 Node3D::GetLocalEulerRotation() const {
    return glm::degrees(glm::eulerAngles(m_LocalRotation));
}

void Node3D::SetRotation(const Quat& InWorldRotation) {
    if (Node3D* parent = GetParentTransform()) {
        m_LocalRotation = glm::normalize(glm::inverse(parent->GetRotation()) * InWorldRotation);
    } else {
        m_LocalRotation = glm::normalize(InWorldRotation);
    }
    TickTransform();
}

void Node3D::SetLocalRotation(const Quat& InLocalRotation) {
    m_LocalRotation = glm::normalize(InLocalRotation);
    TickTransform();
}

void Node3D::SetEulerRotation(const Vec3& InWorldEulerAngles) {
    SetRotation(Quat(glm::radians(InWorldEulerAngles)));
}

void Node3D::SetLocalEulerRotation(const Vec3& InLocalEulerAngles) {
    SetLocalRotation(Quat(glm::radians(InLocalEulerAngles)));
}

void Node3D::Rotate(const Quat& InDeltaRotation) {
    // Self-space rotation, matching Unity's Transform.Rotate (Space.Self).
    SetLocalRotation(m_LocalRotation * InDeltaRotation);
}

void Node3D::RotateEuler(const Vec3& InDeltaEulerAngles) {
    Rotate(Quat(glm::radians(InDeltaEulerAngles)));
}

void Node3D::RotateWithAxis(const Vec3& InAxis, float InAngle) {
    const Quat delta = glm::angleAxis(glm::radians(InAngle), glm::normalize(InAxis));
    SetLocalRotation(m_LocalRotation * delta);
}

void Node3D::SetLookDirection(const Vec3& InWorldDirection, const Vec3& InWorldUpDirection) {
    SetRotation(glm::quatLookAtLH(glm::normalize(InWorldDirection), InWorldUpDirection));
}

/* ---- Scale ---- */

Vec3 Node3D::GetScale() const {
    // Unity-style lossy world scale: component-wise product up the chain.
    // (Ignores shear introduced by non-uniform parent scale + child rotation.)
    Node3D* parent = GetParentTransformChecked();
    return parent ? parent->GetScale() * m_LocalScale : m_LocalScale;
}

Vec3 Node3D::GetLocalScale() const {
    return m_LocalScale;
}

void Node3D::SetScale(const Vec3& InWorldScale) {
    if (Node3D* parent = GetParentTransform()) {
        const Vec3 parentScale = parent->GetScale();
        m_LocalScale = Vec3(
            parentScale.x != 0.0f ? InWorldScale.x / parentScale.x : InWorldScale.x,
            parentScale.y != 0.0f ? InWorldScale.y / parentScale.y : InWorldScale.y,
            parentScale.z != 0.0f ? InWorldScale.z / parentScale.z : InWorldScale.z);
    } else {
        m_LocalScale = InWorldScale;
    }
    TickTransform();
}

void Node3D::SetLocalScale(const Vec3& InLocalScale) {
    m_LocalScale = InLocalScale;
    TickTransform();
}

/* ---- Matrices ---- */

Mat4 Node3D::GetLocalTransformMatrix() const {
    return CalculateTransformMatrix(m_LocalPosition, m_LocalRotation, m_LocalScale);
}

Mat4 Node3D::GetTransformMatrix() const {
    Node3D* parent = GetParentTransformChecked();
    return parent ? parent->GetTransformMatrix() * GetLocalTransformMatrix() : GetLocalTransformMatrix();
}

void Node3D::SetTransformMatrix(const Mat4& InWorldMatrix) {
    // Boundary conversion: a raw world matrix is decomposed into local SRT
    // exactly once, here. Every other accessor stays decomposition-free.
    Mat4 local = InWorldMatrix;
    if (Node3D* parent = GetParentTransform()) {
        local = glm::inverse(parent->GetTransformMatrix()) * InWorldMatrix;
    }

    Vec3 skew;
    Vec4 perspective;
    glm::decompose(local, m_LocalScale, m_LocalRotation, m_LocalPosition, skew, perspective);
    TickTransform();
}

Mat4 Node3D::CalculateTransformMatrix(const Vec3& InPosition, const Vec3& InEulerAngles, const Vec3& InScale) {
    return CalculateTransformMatrix(InPosition, Quat(glm::radians(InEulerAngles)), InScale);
}

Mat4 Node3D::CalculateTransformMatrix(const Vec3& InPosition, const Quat& InRotation, const Vec3& InScale) {
    return glm::translate(Mat4(1.0f), InPosition)
        * glm::toMat4(InRotation)
        * glm::scale(Mat4(1.0f), InScale);
}

Vec3 Node3D::GetRightVector() const {
    return glm::normalize(Vec3(GetTransformMatrix()[0]));
}

Vec3 Node3D::GetUpVector() const {
    return glm::normalize(Vec3(GetTransformMatrix()[1]));
}

Vec3 Node3D::GetForwardVector() const {
    return glm::normalize(Vec3(GetTransformMatrix()[2]));
}

void Node3D::InitializeNode(World& OutWorld) {
    Super::InitializeNode(OutWorld);
}