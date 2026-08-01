#include "GizmoGeometry.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <utility>

static const Vec3 s_LightDirection = glm::normalize(Vec3(0.4f, 0.85f, 0.35f));
static constexpr int32_t s_ConeSegments = 20;
static constexpr int32_t s_DiscSegments = 32;

void GizmoGeometry::Clear() {
    m_Vertices.Clear();
    m_Indices.Clear();
}

void GizmoGeometry::SetViewer(const Vec3& InEyePosition, const Vec3& InViewDirection, bool InOrthographic) {
    m_EyePosition = InEyePosition;
    m_ViewDirection = glm::normalize(InViewDirection);
    m_Orthographic = InOrthographic;
}

void GizmoGeometry::BuildPlaneBasis(const Vec3& InAxis, Vec3& OutU, Vec3& OutV) {
    const Vec3 axis = glm::normalize(InAxis);
    const Vec3 reference = std::abs(axis.y) < 0.9f ? VecUtils::Up : VecUtils::Right;
    OutU = glm::normalize(glm::cross(reference, axis));
    OutV = glm::cross(axis, OutU);
}

Vec3 GizmoGeometry::DirectionToViewer(const Vec3& InPoint) const {
    if (m_Orthographic) {
        return -m_ViewDirection;
    }
    const Vec3 offset = m_EyePosition - InPoint;
    const float length = glm::length(offset);
    return length > 1e-6f ? offset / length : -m_ViewDirection;
}

Vec4 GizmoGeometry::Shade(const Vec4& InColor, const Vec3& InNormal) const {
    const float lambert = glm::max(glm::dot(glm::normalize(InNormal), s_LightDirection), 0.0f);
    return Vec4(Vec3(InColor) * (0.45f + 0.55f * lambert), InColor.a);
}

uint32_t GizmoGeometry::AddVertex(const Vec3& InPosition, const Vec4& InColor) {
    const uint32_t index = (uint32_t)m_Vertices.Size();
    m_Vertices.Add(GizmoVertex{ InPosition, InColor });
    return index;
}

void GizmoGeometry::AddTriangle(const Vec3& InA, const Vec3& InB, const Vec3& InC, const Vec4& InColor) {
    const uint32_t base = AddVertex(InA, InColor);
    AddVertex(InB, InColor);
    AddVertex(InC, InColor);
    m_Indices.Add(base);
    m_Indices.Add(base + 1);
    m_Indices.Add(base + 2);
}

void GizmoGeometry::AddQuad(const Vec3& InA, const Vec3& InB, const Vec3& InC, const Vec3& InD, const Vec4& InColor) {
    AddTriangle(InA, InB, InC, InColor);
    AddTriangle(InA, InC, InD, InColor);
}

void GizmoGeometry::AddShadedQuad(const Vec3& InA, const Vec3& InB, const Vec3& InC, const Vec3& InD, const Vec4& InColor) {
    const Vec3 normal = glm::cross(InB - InA, InC - InA);
    if (glm::dot(normal, normal) < 1e-12f) {
        return;
    }
    AddQuad(InA, InB, InC, InD, Shade(InColor, normal));
}

Vec3 GizmoGeometry::SideOffset(const Vec3& InPoint, const Vec3& InTangent, float InHalfWidth) const {
    Vec3 side = glm::cross(InTangent, DirectionToViewer(InPoint));
    float length = glm::length(side);
    if (length < 1e-6f) {
        Vec3 fallbackU, fallbackV;
        BuildPlaneBasis(InTangent, fallbackU, fallbackV);
        side = fallbackU;
        length = 1.0f;
    }
    return side * (InHalfWidth / length);
}

void GizmoGeometry::AddRibbon(const Array<Vec3>& InPoints, bool InClosed, float InHalfWidth, const Vec4& InColor) {
    const int32_t count = InPoints.Size();
    if (count < 2) {
        return;
    }

    Array<Vec3> sides;
    for (int32_t i = 0; i < count; i++) {
        const Vec3& previous = InPoints[i > 0 ? i - 1 : (InClosed ? count - 1 : 0)];
        const Vec3& next = InPoints[i < count - 1 ? i + 1 : (InClosed ? 0 : count - 1)];
        Vec3 tangent = next - previous;
        if (glm::dot(tangent, tangent) < 1e-12f) {
            tangent = Vec3(1.0f, 0.0f, 0.0f);
        }
        sides.Add(SideOffset(InPoints[i], glm::normalize(tangent), InHalfWidth));
    }

    const int32_t segments = InClosed ? count : count - 1;
    for (int32_t i = 0; i < segments; i++) {
        const int32_t next = (i + 1) % count;
        AddQuad(InPoints[i] - sides[i], InPoints[next] - sides[next],
                InPoints[next] + sides[next], InPoints[i] + sides[i], InColor);
    }
}

void GizmoGeometry::AddLine(const Vec3& InFrom, const Vec3& InTo, float InHalfWidth, const Vec4& InColor) {
    AddRibbon({ InFrom, InTo }, false, InHalfWidth, InColor);
}

void GizmoGeometry::AddDisc(const Vec3& InCenter, float InRadius, const Vec4& InColor) {
    Vec3 axisU, axisV;
    BuildPlaneBasis(DirectionToViewer(InCenter), axisU, axisV);

    for (int32_t i = 0; i < s_DiscSegments; i++) {
        const float angle = glm::two_pi<float>() * (float)i / (float)s_DiscSegments;
        const float nextAngle = glm::two_pi<float>() * (float)(i + 1) / (float)s_DiscSegments;
        const Vec3 current = InCenter + (axisU * std::cos(angle) + axisV * std::sin(angle)) * InRadius;
        const Vec3 next = InCenter + (axisU * std::cos(nextAngle) + axisV * std::sin(nextAngle)) * InRadius;
        AddTriangle(InCenter, current, next, InColor);
    }
}

void GizmoGeometry::AddCone(const Vec3& InBase, const Vec3& InTip, float InRadius, const Vec4& InColor) {
    const Vec3 axis = InTip - InBase;
    const float length = glm::length(axis);
    if (length < 1e-6f) {
        return;
    }
    const Vec3 direction = axis / length;
    Vec3 axisU, axisV;
    BuildPlaneBasis(direction, axisU, axisV);

    for (int32_t i = 0; i < s_ConeSegments; i++) {
        const float angle = glm::two_pi<float>() * (float)i / (float)s_ConeSegments;
        const float nextAngle = glm::two_pi<float>() * (float)(i + 1) / (float)s_ConeSegments;
        const Vec3 current = InBase + (axisU * std::cos(angle) + axisV * std::sin(angle)) * InRadius;
        const Vec3 next = InBase + (axisU * std::cos(nextAngle) + axisV * std::sin(nextAngle)) * InRadius;
        AddTriangle(current, next, InTip, Shade(InColor, glm::cross(next - current, InTip - current)));
        AddTriangle(InBase, next, current, Shade(InColor, -direction));
    }
}

void GizmoGeometry::AddBox(const Vec3& InCenter, const Vec3& InHalfX, const Vec3& InHalfY, const Vec3& InHalfZ, const Vec4& InColor) {
    const Vec3 halves[3] = { InHalfX, InHalfY, InHalfZ };
    for (int32_t axis = 0; axis < 3; axis++) {
        const Vec3& normal = halves[axis];
        const Vec3& tangent = halves[(axis + 1) % 3];
        const Vec3& bitangent = halves[(axis + 2) % 3];
        for (int32_t side = 0; side < 2; side++) {
            const Vec3 face = InCenter + (side == 0 ? normal : -normal);
            AddShadedQuad(face - tangent - bitangent, face + tangent - bitangent,
                          face + tangent + bitangent, face - tangent + bitangent, InColor);
        }
    }
}

void GizmoGeometry::SortForPainter() {
    const int32_t triangleCount = m_Indices.Size() / 3;
    if (triangleCount < 2) {
        return;
    }

    Array<std::pair<float, int32_t>> order;
    for (int32_t triangle = 0; triangle < triangleCount; triangle++) {
        const Vec3 centroid = (m_Vertices[m_Indices[triangle * 3]].Position
                             + m_Vertices[m_Indices[triangle * 3 + 1]].Position
                             + m_Vertices[m_Indices[triangle * 3 + 2]].Position) / 3.0f;
        order.Add({ glm::dot(centroid - m_EyePosition, m_ViewDirection), triangle });
    }
    order.Sort([](const std::pair<float, int32_t>& InA, const std::pair<float, int32_t>& InB) {
        return InA.first > InB.first;
    });

    Array<uint32_t> sorted;
    sorted.Resize(m_Indices.Size());
    for (int32_t i = 0; i < triangleCount; i++) {
        const int32_t source = order[i].second;
        sorted[i * 3] = m_Indices[source * 3];
        sorted[i * 3 + 1] = m_Indices[source * 3 + 1];
        sorted[i * 3 + 2] = m_Indices[source * 3 + 2];
    }
    m_Indices = sorted;
}
