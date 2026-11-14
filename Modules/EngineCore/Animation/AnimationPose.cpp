#include "AnimationPose.h"
#include "GameFramework/Node.h"
#include "Object/Property.h"
#include "Serialization/Json.h"
#include <cmath>

using json = nlohmann::json;

static const char* s_RotationAxes[] = { "X", "Y", "Z", "W" };

/** Numbers blend, everything else holds the earlier value until the next key takes over. */
static json BlendValue(const json& InA, const json& InB, float InAlpha) {
    if (InA.is_number() && InB.is_number()) {
        const double blended = InA.get<double>() + (InB.get<double>() - InA.get<double>()) * (double)InAlpha;
        if (InA.is_number_float() || InB.is_number_float()) {
            return blended;
        }
        return (int64_t)std::llround(blended);
    }
    if (InA.is_object() && InB.is_object()) {
        json result = InA;
        for (const auto& [name, value] : InA.items()) {
            if (InB.contains(name)) {
                result[name] = BlendValue(value, InB[name], InAlpha);
            }
        }
        return result;
    }
    if (InA.is_array() && InB.is_array() && InA.size() == InB.size()) {
        json result = InA;
        for (size_t i = 0; i < InA.size(); i++) {
            result[i] = BlendValue(InA[i], InB[i], InAlpha);
        }
        return result;
    }
    return InA;
}

static json BlendRotation(const json& InA, const json& InB, float InAlpha) {
    double from[4], to[4], dot = 0.0;
    for (int32_t i = 0; i < 4; i++) {
        if (!InA[s_RotationAxes[i]].is_number() || !InB[s_RotationAxes[i]].is_number()) {
            return BlendValue(InA, InB, InAlpha);
        }
        from[i] = InA[s_RotationAxes[i]].get<double>();
        to[i] = InB[s_RotationAxes[i]].get<double>();
        dot += from[i] * to[i];
    }

    const double shortest = dot < 0.0 ? -1.0 : 1.0;
    double blended[4], length = 0.0;
    for (int32_t i = 0; i < 4; i++) {
        blended[i] = from[i] + (to[i] * shortest - from[i]) * (double)InAlpha;
        length += blended[i] * blended[i];
    }
    length = std::sqrt(length);
    if (length <= 0.0) {
        return InA;
    }

    json result = InA;
    for (int32_t i = 0; i < 4; i++) {
        result[s_RotationAxes[i]] = blended[i] / length;
    }
    return result;
}

static bool IsRotation(Property& InMember) {
    StructProperty* structProperty = Cast<StructProperty>(&InMember);
    return structProperty && structProperty->InnerStructTypename == "Quat";
}

json AnimationPose::Blend(Property& InMember, const json& InFrom, const json& InTo, float InAlpha) {
    return IsRotation(InMember) && InFrom.is_object() && InTo.is_object() ? BlendRotation(InFrom, InTo, InAlpha)
                                                                          : BlendValue(InFrom, InTo, InAlpha);
}

AnimationPose::Channel* AnimationPose::Find(const Node& InTarget, const Property& InMember) {
    for (Channel& channel : m_Channels) {
        if (channel.Target == &InTarget && channel.Member == &InMember) {
            return &channel;
        }
    }
    return nullptr;
}

void AnimationPose::Set(Node& InTarget, Property& InMember, json InValue) {
    if (Channel* existing = Find(InTarget, InMember)) {
        existing->Value = std::move(InValue);
        return;
    }
    m_Channels.Add(Channel{ &InTarget, &InMember, std::move(InValue) });
}

void AnimationPose::BlendTowards(const AnimationPose& InOther, float InAlpha) {
    for (const Channel& other : InOther.m_Channels) {
        if (Channel* channel = Find(*other.Target, *other.Member)) {
            channel->Value = Blend(*other.Member, channel->Value, other.Value, InAlpha);
            continue;
        }
        const json current = JsonSerializer::SerializeProperty(other.Member, other.Member->GetValuePtr(other.Target));
        m_Channels.Add(Channel{ other.Target, other.Member, Blend(*other.Member, current, other.Value, InAlpha) });
    }
}

void AnimationPose::Apply() const {
    for (const Channel& channel : m_Channels) {
        try {
            JsonSerializer::DeserializeProperty(channel.Member, channel.Member->GetValuePtr(channel.Target), channel.Value);
        } catch (const std::exception&) {
            continue;
        }
        channel.Member->NotifyChanged(channel.Target);
    }
}
