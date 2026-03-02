#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>

using byte = uint8_t;

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

namespace VecUtils {
    inline const Vec3 Zero = Vec3(0.0f, 0.0f, 0.0f);
    inline const Vec3 One = Vec3(1.0f, 1.0f, 1.0f);
    inline const Vec3 Up = Vec3(0.0f, 1.0f, 0.0f);
    inline const Vec3 Down = Vec3(0.0f, -1.0f, 0.0f);
    inline const Vec3 Right = Vec3(1.0f, 0.0f, 0.0f);
    inline const Vec3 Left = Vec3(-1.0f, 0.0f, 0.0f);
    inline const Vec3 Forward = Vec3(0.0f, 0.0f, -1.0f);
    inline const Vec3 Backward = Vec3(0.0f, 0.0f, 1.0f);
}
