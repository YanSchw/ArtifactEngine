#include "AudioPlayParams.h"

#include <algorithm>
#include <cmath>

float AudioAttenuation::Evaluate(float InDistance) const {
    const float minDistance = std::max(MinDistance, 0.0001f);
    const float maxDistance = std::max(MaxDistance, minDistance);
    const float distance = std::clamp(InDistance, minDistance, maxDistance);

    switch (Model) {
        case AudioAttenuationModel::None:
            return 1.0f;
        case AudioAttenuationModel::Linear:
            return maxDistance > minDistance ? 1.0f - std::clamp(Rolloff * (distance - minDistance) / (maxDistance - minDistance), 0.0f, 1.0f) : 1.0f;
        case AudioAttenuationModel::Inverse:
            return minDistance / (minDistance + std::max(Rolloff, 0.0f) * (distance - minDistance));
        case AudioAttenuationModel::Exponential:
            return std::pow(distance / minDistance, -std::max(Rolloff, 0.0f));
    }
    return 1.0f;
}
