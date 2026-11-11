#pragma once
#include "Node.h"
#include "Common/Types.h"
#include "Object/Pointer.h"
#include "Object/Struct.h"
#include "SkyboxNode.gen.h"

class Texture2D;
class World;

ARTIFACT_ENUM();
enum class SkyboxMode : uint8_t {
    Gradient = 0,
    Hdri = 1
};

struct SkyGradientParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    Color Top = Color(0.2f, 0.38f, 0.68f, 1.0f);

    PROPERTY()
    Color Horizon = Color(0.66f, 0.74f, 0.82f, 1.0f);

    PROPERTY()
    Color Bottom = Color(0.16f, 0.15f, 0.14f, 1.0f);
};

struct SkyHdriParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    WeakObjectPtr<Texture2D> Texture;

    PROPERTY()
    float Rotation = 0.0f;
};

class SkyboxNode : public Node {
public:
    ARTIFACT_CLASS();

    static SkyboxNode* FindActive(World* InWorld);

    FUNCTION(Pure)
    SkyboxMode GetMode() const { return m_Mode; }
    FUNCTION()
    void SetMode(SkyboxMode InMode) { m_Mode = InMode; }

    FUNCTION(Pure)
    float GetIntensity() const { return m_Intensity; }
    FUNCTION()
    void SetIntensity(float InIntensity) { m_Intensity = InIntensity; }

    const SkyGradientParams& GetGradient() const { return m_Gradient; }
    SkyGradientParams& GetGradient() { return m_Gradient; }

    const SkyHdriParams& GetHdri() const { return m_Hdri; }
    SkyHdriParams& GetHdri() { return m_Hdri; }

private:
    PROPERTY()
    SkyboxMode m_Mode = SkyboxMode::Gradient;

    PROPERTY()
    float m_Intensity = 1.0f;

    PROPERTY()
    SkyGradientParams m_Gradient;

    PROPERTY()
    SkyHdriParams m_Hdri;
};
