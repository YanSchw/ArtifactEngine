#pragma once
#include "Node3D.h"
#include "PointLightNode.gen.h"

/** An omnidirectional light with a quadratic falloff that reaches zero at its radius. */
class PointLightNode : public Node3D {
public:
    ARTIFACT_CLASS();

    FUNCTION(Pure)
    Color GetColor() const { return m_Color; }
    FUNCTION()
    void SetColor(const Color& InColor) { m_Color = InColor; }

    FUNCTION(Pure)
    float GetIntensity() const { return m_Intensity; }
    FUNCTION()
    void SetIntensity(float InIntensity) { m_Intensity = InIntensity; }

    FUNCTION(Pure)
    float GetRadius() const { return m_Radius; }
    FUNCTION()
    void SetRadius(float InRadius) { m_Radius = InRadius; }

private:
    PROPERTY()
    Color m_Color = Color(1.0f, 0.85f, 0.7f, 1.0f);

    PROPERTY()
    float m_Intensity = 5.0f;

    /** Distance at which the light has fully faded out. */
    PROPERTY()
    float m_Radius = 12.0f;
};
