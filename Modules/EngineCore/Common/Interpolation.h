#pragma once
#include "Common/Types.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace Math {

    /** Linear interpolation, unclamped: an alpha of 0 gives InA, 1 gives InB. */
    template <typename T>
    inline T Lerp(const T& InA, const T& InB, float InAlpha) {
        return InA + (InB - InA) * InAlpha;
    }

    /** Rotations interpolate along the shortest arc at constant angular speed. */
    inline Quat Lerp(const Quat& InA, const Quat& InB, float InAlpha) {
        return glm::slerp(InA, InB, InAlpha);
    }

    /** The alpha at which Lerp(InA, InB, alpha) gives InValue, unclamped. 0 when InA == InB. */
    inline float InverseLerp(float InA, float InB, float InValue) {
        return InA != InB ? (InValue - InA) / (InB - InA) : 0.0f;
    }

    /** Maps InValue from [InFromMin, InFromMax] onto [InToMin, InToMax], unclamped. */
    inline float Remap(float InValue, float InFromMin, float InFromMax, float InToMin, float InToMax) {
        return Lerp(InToMin, InToMax, InverseLerp(InFromMin, InFromMax, InValue));
    }

    /** Remap that stays inside [InToMin, InToMax]. */
    inline float RemapClamped(float InValue, float InFromMin, float InFromMax, float InToMin, float InToMax) {
        return Lerp(InToMin, InToMax, std::clamp(InverseLerp(InFromMin, InFromMax, InValue), 0.0f, 1.0f));
    }

    /** 0 below InEdge0, 1 above InEdge1, an S-curve in between. */
    inline float SmoothStep(float InEdge0, float InEdge1, float InValue) {
        const float t = std::clamp(InverseLerp(InEdge0, InEdge1, InValue), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    /** Shortest signed turn from InFrom to InTo in degrees, in [-180, 180). */
    inline float DeltaAngle(float InFrom, float InTo) {
        float delta = std::fmod(InTo - InFrom, 360.0f);
        if (delta >= 180.0f) {
            delta -= 360.0f;
        } else if (delta < -180.0f) {
            delta += 360.0f;
        }
        return delta;
    }

    /** Lerp for angles in degrees that turns the short way round (350 to 10 passes through 0). */
    inline float LerpAngle(float InA, float InB, float InAlpha) {
        return InA + DeltaAngle(InA, InB) * InAlpha;
    }

    /** The alpha for one frame of Damp: the remaining distance halves InSpeed times per second. */
    inline float DampAlpha(float InSpeed, float InDeltaTime) {
        return 1.0f - std::exp2(-InSpeed * InDeltaTime);
    }

    /** Eases InCurrent towards InTarget, fast at first and slowing down as it gets close.
     *  InSpeed is how many times per second the remaining distance halves. */
    template <typename T>
    inline T Damp(const T& InCurrent, const T& InTarget, float InSpeed, float InDeltaTime) {
        return Lerp(InCurrent, InTarget, DampAlpha(InSpeed, InDeltaTime));
    }

    /** Damp for angles in degrees, turning the short way round. */
    inline float DampAngle(float InCurrent, float InTarget, float InSpeed, float InDeltaTime) {
        return LerpAngle(InCurrent, InTarget, DampAlpha(InSpeed, InDeltaTime));
    }

    /** Follows InTarget like a critically damped spring: starts and stops smoothly, even when
     *  the target keeps moving. InOutVelocity carries the motion between frames, so keep it in
     *  a member that starts at zero. InSmoothTime is roughly how long it takes to catch up.
     *  For float and vectors. */
    template <typename T>
    inline T SmoothDamp(const T& InCurrent, const T& InTarget, T& InOutVelocity, float InSmoothTime, float InDeltaTime) {
        const float omega = 2.0f / std::max(InSmoothTime, 0.0001f);
        const float x = omega * InDeltaTime;
        const float decay = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
        const T offset = InCurrent - InTarget;
        const T change = (InOutVelocity + offset * omega) * InDeltaTime;
        InOutVelocity = (InOutVelocity - change * omega) * decay;
        return InTarget + (offset + change) * decay;
    }

    /** Steps InCurrent towards InTarget by at most InMaxDelta without overshooting.
     *  For a constant speed pass speed * deltaTime. */
    inline float MoveTowards(float InCurrent, float InTarget, float InMaxDelta) {
        const float delta = InTarget - InCurrent;
        return std::abs(delta) <= InMaxDelta ? InTarget : InCurrent + std::copysign(InMaxDelta, delta);
    }

    template <glm::length_t L, glm::qualifier Q>
    inline glm::vec<L, float, Q> MoveTowards(const glm::vec<L, float, Q>& InCurrent, const glm::vec<L, float, Q>& InTarget, float InMaxDelta) {
        const glm::vec<L, float, Q> delta = InTarget - InCurrent;
        const float distance = glm::length(delta);
        return distance <= InMaxDelta ? InTarget : InCurrent + delta * (InMaxDelta / distance);
    }

    /** Point on the quadratic Bezier curve from InP0 to InP2, pulled towards InP1. */
    template <typename T>
    inline T Bezier(const T& InP0, const T& InP1, const T& InP2, float InAlpha) {
        const float u = 1.0f - InAlpha;
        return InP0 * (u * u) + InP1 * (2.0f * u * InAlpha) + InP2 * (InAlpha * InAlpha);
    }

    /** Point on the cubic Bezier curve from InP0 to InP3, pulled towards InP1 and InP2. */
    template <typename T>
    inline T Bezier(const T& InP0, const T& InP1, const T& InP2, const T& InP3, float InAlpha) {
        const float u = 1.0f - InAlpha;
        return InP0 * (u * u * u) + InP1 * (3.0f * u * u * InAlpha) +
               InP2 * (3.0f * u * InAlpha * InAlpha) + InP3 * (InAlpha * InAlpha * InAlpha);
    }

    /** Point on the smooth curve through InP1 (alpha 0) and InP2 (alpha 1), shaped by the
     *  neighbouring points InP0 and InP3. Chain it over a list of points for a smooth path. */
    template <typename T>
    inline T CatmullRom(const T& InP0, const T& InP1, const T& InP2, const T& InP3, float InAlpha) {
        const float t2 = InAlpha * InAlpha;
        const float t3 = t2 * InAlpha;
        return (InP1 * 2.0f +
                (InP2 - InP0) * InAlpha +
                (InP0 * 2.0f - InP1 * 5.0f + InP2 * 4.0f - InP3) * t2 +
                (InP1 * 3.0f - InP0 - InP2 * 3.0f + InP3) * t3) * 0.5f;
    }

    /** Easing curves: take an alpha in [0, 1] and return the eased alpha, 0 at 0 and 1 at 1.
     *  Feed the result into Lerp: Math::Lerp(InFrom, InTo, Math::Ease::OutCubic(t)). */
    namespace Ease {
        inline float InQuad(float t) { return t * t; }
        inline float OutQuad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
        inline float InOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t); }

        inline float InCubic(float t) { return t * t * t; }
        inline float OutCubic(float t) { const float u = 1.0f - t; return 1.0f - u * u * u; }
        inline float InOutCubic(float t) { const float u = 1.0f - t; return t < 0.5f ? 4.0f * t * t * t : 1.0f - 4.0f * u * u * u; }

        inline float InSine(float t) { return 1.0f - std::cos(t * glm::half_pi<float>()); }
        inline float OutSine(float t) { return std::sin(t * glm::half_pi<float>()); }
        inline float InOutSine(float t) { return 0.5f * (1.0f - std::cos(t * glm::pi<float>())); }

        /** Pulls back a little before moving (In) or shoots a little past the end and settles (Out). */
        inline float InBack(float t) { return 2.70158f * t * t * t - 1.70158f * t * t; }
        inline float OutBack(float t) { const float u = t - 1.0f; return 1.0f + 2.70158f * u * u * u + 1.70158f * u * u; }

        /** Overshoots and wobbles around the end like a spring. */
        inline float OutElastic(float t) {
            if (t <= 0.0f || t >= 1.0f) {
                return t <= 0.0f ? 0.0f : 1.0f;
            }
            return std::exp2(-10.0f * t) * std::sin((t * 10.0f - 0.75f) * (glm::two_pi<float>() / 3.0f)) + 1.0f;
        }

        /** Hits the end and bounces a few times, like a dropped ball. */
        inline float OutBounce(float t) {
            constexpr float n = 7.5625f;
            constexpr float d = 2.75f;
            if (t < 1.0f / d) {
                return n * t * t;
            }
            if (t < 2.0f / d) {
                t -= 1.5f / d;
                return n * t * t + 0.75f;
            }
            if (t < 2.5f / d) {
                t -= 2.25f / d;
                return n * t * t + 0.9375f;
            }
            t -= 2.625f / d;
            return n * t * t + 0.984375f;
        }
    }
}
