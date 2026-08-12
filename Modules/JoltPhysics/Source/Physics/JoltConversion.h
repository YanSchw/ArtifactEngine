#pragma once
#include "Common/Types.h"
#include <Jolt/Jolt.h>

inline JPH::Vec3 ToJolt(const Vec3& InVector) {
    return JPH::Vec3(InVector.x, InVector.y, InVector.z);
}

inline JPH::Quat ToJolt(const Quat& InRotation) {
    return JPH::Quat(InRotation.x, InRotation.y, InRotation.z, InRotation.w);
}

inline Vec3 FromJolt(const JPH::Vec3& InVector) {
    return Vec3(InVector.GetX(), InVector.GetY(), InVector.GetZ());
}

inline Quat FromJolt(const JPH::Quat& InRotation) {
    return Quat(InRotation.GetW(), InRotation.GetX(), InRotation.GetY(), InRotation.GetZ());
}
