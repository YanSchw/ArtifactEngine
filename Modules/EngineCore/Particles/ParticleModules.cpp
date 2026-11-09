#include "ParticleModules.h"

template<typename T, typename TKey>
static T EvaluateKeys(const Array<TKey>& InKeys, ParticleCurveInterpolation InInterpolation, float InTime, const T& InDefault) {
    const TKey* before = nullptr;
    const TKey* after = nullptr;
    for (const TKey& key : InKeys) {
        if (key.Time <= InTime && (!before || key.Time >= before->Time)) {
            before = &key;
        }
        if (key.Time >= InTime && (!after || key.Time < after->Time)) {
            after = &key;
        }
    }
    if (!before || !after) {
        return before ? before->Value : after ? after->Value : InDefault;
    }

    const float span = after->Time - before->Time;
    float alpha = span > 1e-6f ? (InTime - before->Time) / span : 0.0f;
    if (InInterpolation == ParticleCurveInterpolation::Smooth) {
        alpha = alpha * alpha * (3.0f - 2.0f * alpha);
    }
    return before->Value + (after->Value - before->Value) * alpha;
}

float ParticleCurve::Evaluate(float InTime) const {
    return EvaluateKeys(Keys, Interpolation, InTime, 1.0f);
}

Color ParticleGradient::Evaluate(float InTime) const {
    return EvaluateKeys(Keys, Interpolation, InTime, Color(1.0f));
}
