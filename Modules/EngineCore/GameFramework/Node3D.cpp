#include "Node3D.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

Node3D* Node3D::GetTransform() {
    return this;
}

Vec3 Node3D::GetPosition() const {
	return Vec3(GetTransformMatrix()[3]);
}

Vec3 Node3D::GetLocalPosition() const {
	return Vec3(m_LocalTransformMatrix[3]);
}

void Node3D::SetPosition(const Vec3& InPosition) {
	const auto& scaled = glm::scale(Mat4(1), GetScale());
	const auto& rotated = glm::toMat4(GetRotation()) * scaled;
	m_WorldTransformMatrix = glm::translate(Mat4(1), InPosition) * rotated;
	TickTransform(true);
}

void Node3D::SetLocalPosition(const Vec3& InPosition) {
	const auto& scaled = glm::scale(Mat4(1), GetLocalScale());
	const auto& rotated = glm::toMat4(GetLocalRotation()) * scaled;
	m_LocalTransformMatrix = glm::translate(Mat4(1), InPosition) * rotated;
	ReprojectLocalMatrixToWorld();
	TickTransform(true);
}

void Node3D::AddWorldOffset(const Vec3& InOffset) {
	SetPosition(GetPosition() + InOffset);
}

void Node3D::AddLocalOffset(const Vec3& InOffset) {
	SetLocalPosition(GetLocalPosition() + InOffset);
}


Quat Node3D::GetRotation() const {
	Vec3 scale;
	glm::quat rotation;
	Vec3 translation;
	Vec3 skew;
	Vec4 perspective;
	glm::decompose(m_WorldTransformMatrix, scale, rotation, translation, skew, perspective);
	return rotation;
}

Quat Node3D::GetLocalRotation() const {
	Vec3 scale;
	glm::quat rotation;
	Vec3 translation;
	Vec3 skew;
	Vec4 perspective;
	glm::decompose(m_LocalTransformMatrix, scale, rotation, translation, skew, perspective);
	return rotation;
}

Vec3 Node3D::GetEulerRotation() const {
	return glm::degrees(glm::eulerAngles(GetRotation()));
}

Vec3 Node3D::GetLocalEulerRotation() const {
	return glm::degrees(glm::eulerAngles(GetLocalRotation()));
}

void Node3D::SetRotation(const Quat& InRotation) {
	const auto& scaled = glm::scale(Mat4(1), GetScale());
	const auto& rotated = glm::toMat4(InRotation) * scaled;
	m_WorldTransformMatrix = glm::translate(Mat4(1), GetPosition()) * rotated;
	TickTransform(true);
}

void Node3D::SetLocalRotation(const Quat& InRotation) {
	const auto& scaled = glm::scale(Mat4(1), GetLocalScale());
	const auto& rotated = glm::toMat4(InRotation) * scaled;
	m_LocalTransformMatrix = glm::translate(Mat4(1), GetLocalPosition()) * rotated;
	ReprojectLocalMatrixToWorld();
	TickTransform(true);
}

void Node3D::SetEulerRotation(const Vec3& InEulerAngles) {
	SetRotation(Quat(glm::radians(InEulerAngles)));
}

void Node3D::SetLocalEulerRotation(const Vec3& InEulerAngles) {
	SetLocalRotation(Quat(glm::radians(InEulerAngles)));
}

void Node3D::Rotate(const Quat& InQuat) {
	// TODO: Double check this 
	SetRotation(GetRotation() * InQuat);
}

void Node3D::RotateEuler(const Vec3& InEulerAngles) {
	SetRotation(glm::normalize(GetRotation() * glm::quat(InEulerAngles)));
}

void Node3D::RotateWithAxis(const Vec3& InAxis, float InAngle) {
	m_WorldTransformMatrix = glm::rotate(m_WorldTransformMatrix, glm::radians(InAngle), InAxis);
	TickTransform(true);
}

void Node3D::SetLookDirection(const Vec3& InDirection, const Vec3& InUpDirection) {
	SetRotation(glm::quatLookAtLH(InDirection, InUpDirection));
}

Vec3 Node3D::GetScale() const {
	Vec3 scale;
	glm::quat rotation;
	Vec3 translation;
	Vec3 skew;
	Vec4 perspective;
	glm::decompose(m_WorldTransformMatrix, scale, rotation, translation, skew, perspective);
	return scale;
}

Vec3 Node3D::GetLocalScale() const {
	Vec3 scale;
	glm::quat rotation;
	Vec3 translation;
	Vec3 skew;
	Vec4 perspective;
	glm::decompose(m_LocalTransformMatrix, scale, rotation, translation, skew, perspective);
	return scale;
}

void Node3D::SetScale(const Vec3& InScale) {
	if (InScale.x <= 0.0f || InScale.y <= 0.0f || InScale.z <= 0.0f) return;
	const auto& scaled = glm::scale(Mat4(1), InScale);
	const auto& rotated = glm::toMat4(GetRotation()) * scaled;
	m_WorldTransformMatrix = glm::translate(Mat4(1), GetPosition()) * rotated;
	TickTransform(true);
}

void Node3D::SetLocalScale(const Vec3& InScale) {
	if (InScale.x <= 0.0f || InScale.y <= 0.0f || InScale.z <= 0.0f) return;
	const auto& scaled = glm::scale(Mat4(1), InScale);
	const auto& rotated = glm::toMat4(GetLocalRotation()) * scaled;
	m_LocalTransformMatrix = glm::translate(Mat4(1), GetLocalPosition()) * rotated;
	ReprojectLocalMatrixToWorld();
	TickTransform(true);
}

Mat4 Node3D::GetTransformMatrix() const {
	return m_WorldTransformMatrix;
}

void Node3D::SetTransformMatrix(const Mat4& mat) {
    m_WorldTransformMatrix = mat;
    RecalculateTransformMatrix();
    TickTransform(true);
}

void Node3D::RecalculateTransformMatrix() {
	bool inverseParentTransform = false;
	/// Source: https://stackoverflow.com/questions/11920866/global-transform-to-local-transform
	m_LocalTransformMatrix = GetParentTransform() ?
		((!inverseParentTransform ? glm::inverse(GetParentTransform()->GetTransformMatrix()) : GetParentTransform()->GetTransformMatrix()) * m_WorldTransformMatrix) :
		m_WorldTransformMatrix;
}

void Node3D::ReprojectLocalMatrixToWorld() {
	if (!GetParentTransform()){
		m_WorldTransformMatrix = m_LocalTransformMatrix;
		return;
	}
	m_WorldTransformMatrix = GetParentTransform()->m_WorldTransformMatrix * m_LocalTransformMatrix;
}

Mat4 Node3D::CalculateTransformMatrix(const Vec3& InPosition, const Vec3& InEulerAngles, const Vec3& InScale) {
	const Mat4 rotation = glm::toMat4(Quat({ glm::radians(InEulerAngles.x), glm::radians(InEulerAngles.y), glm::radians(InEulerAngles.z) }));
	return glm::translate(Mat4(1.0f), { InPosition.x, InPosition.y, InPosition.z })
		* rotation
		* glm::scale(Mat4(1.0f), { InScale.x, InScale.y, InScale.z });
}

Mat4 Node3D::CalculateTransformMatrix(const Vec3& InPosition, const Quat& InRotation, const Vec3& InScale) {
	const Mat4 rot = glm::toMat4(InRotation);
	return glm::translate(Mat4(1.0f), { InPosition.x, InPosition.y, InPosition.z })
		* rot
		* glm::scale(Mat4(1.0f), { InScale.x, InScale.y, InScale.z });
}

Vec3 Node3D::GetRightVector() const {
	//const Mat4 inverted = glm::inverse(GetTransformMatrix());
	const Vec3 right = glm::normalize(Vec3(GetTransformMatrix()[0]));
	return right;
}

Vec3 Node3D::GetUpVector() const {
	//const Mat4 inverted = glm::inverse(GetTransformMatrix());
	const Vec3 up = glm::normalize(Vec3(GetTransformMatrix()[1]));
	return up;
}

Vec3 Node3D::GetForwardVector() const {
	//const Mat4 inverted = glm::inverse(GetTransformMatrix());
	const Vec3 forward = glm::normalize(Vec3(GetTransformMatrix()[2]));
	return forward;
}

void Node3D::TickTransform(bool InInWorldSpace) {
	if (!InInWorldSpace) {
		ReprojectLocalMatrixToWorld();
	} else{
		RecalculateTransformMatrix();
	}
	Super::TickTransform(InInWorldSpace);
	RecalculateTransformMatrix();
}

void Node3D::InitializeNode(World& OutWorld) {
	Super::InitializeNode(OutWorld);
}