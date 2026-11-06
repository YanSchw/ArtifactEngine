#pragma once
#include "Common/Types.h"
#include "Object/Enum.h"
#include "Object/Struct.h"
#include "AudioPlayParams.gen.h"

class World;

ARTIFACT_ENUM();
enum class AudioAttenuationModel : uint8_t {
    None = 0,
    Linear = 1,
    Inverse = 2,
    Exponential = 3
};

struct AudioAttenuation {
    ARTIFACT_STRUCT();

    PROPERTY()
    AudioAttenuationModel Model = AudioAttenuationModel::Inverse;

    PROPERTY()
    float MinDistance = 1.0f;

    PROPERTY()
    float MaxDistance = 50.0f;

    PROPERTY()
    float Rolloff = 1.0f;

    float Evaluate(float InDistance) const;
};

struct AudioPlayParams {
    float Volume = 1.0f;
    float Pitch = 1.0f;
    float Pan = 0.0f;
    bool Loop = false;
    bool StartPaused = false;
    float FadeInTime = 0.0f;
    double StartTime = 0.0;
    String Bus = "Master";

    bool Spatial = false;
    Vec3 Position = Vec3(0.0f);
    Vec3 Velocity = Vec3(0.0f);
    AudioAttenuation Attenuation;

    World* OwningWorld = nullptr;
};
