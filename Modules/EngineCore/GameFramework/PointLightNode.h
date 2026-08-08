#pragma once
#include "Node3D.h"
#include "PointLightNode.gen.h"

/** An omnidirectional light with a quadratic falloff that reaches zero at its radius. */
class PointLightNode : public Node3D {
public:
    ARTIFACT_CLASS();

    Vec3 GetColor() const { return m_Color; }
    void SetColor(const Vec3& InColor) { m_Color = InColor; }

    float GetIntensity() const { return m_Intensity; }
    void SetIntensity(float InIntensity) { m_Intensity = InIntensity; }

    float GetRadius() const { return m_Radius; }
    void SetRadius(float InRadius) { m_Radius = InRadius; }

private:
    PROPERTY()
    Vec3 m_Color = Vec3(1.0f, 0.85f, 0.7f);

    PROPERTY()
    float m_Intensity = 5.0f;

    /** Distance at which the light has fully faded out. */
    PROPERTY()
    float m_Radius = 12.0f;
};
