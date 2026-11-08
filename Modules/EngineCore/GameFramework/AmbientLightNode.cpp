#include "AmbientLightNode.h"

void AmbientLightNode::AccumulateCube(Color (&InOutCube)[SceneUniformData::AmbientCubeFaces]) const {
    const Color faces[SceneUniformData::AmbientCubeFaces] = { m_East, m_West, m_Top, m_Bottom, m_North, m_South };
    for (int32_t face = 0; face < SceneUniformData::AmbientCubeFaces; face++) {
        InOutCube[face] += Color(Vec3(faces[face]) * m_Intensity, 0.0f);
    }
}
