#pragma once
#include "Node.h"
#include "Rendering/SceneUniforms.h"
#include "AmbientLightNode.gen.h"

class AmbientLightNode : public Node {
public:
    ARTIFACT_CLASS();

    void AccumulateCube(Color (&InOutCube)[SceneUniformData::AmbientCubeFaces]) const;

    float GetIntensity() const { return m_Intensity; }
    void SetIntensity(float InIntensity) { m_Intensity = InIntensity; }

private:
    PROPERTY()
    Color m_Top = Color(0.55f, 0.65f, 0.8f, 1.0f);

    PROPERTY()
    Color m_Bottom = Color(0.27f, 0.25f, 0.23f, 1.0f);

    PROPERTY()
    Color m_North = Color(0.42f, 0.47f, 0.53f, 1.0f);

    PROPERTY()
    Color m_East = Color(0.42f, 0.47f, 0.53f, 1.0f);

    PROPERTY()
    Color m_South = Color(0.42f, 0.47f, 0.53f, 1.0f);

    PROPERTY()
    Color m_West = Color(0.42f, 0.47f, 0.53f, 1.0f);

    PROPERTY()
    float m_Intensity = 0.35f;
};
