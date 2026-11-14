#pragma once
#include "Common/Array.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"

class Node;
struct Property;

class AnimationPose {
public:
    struct Channel {
        Node* Target = nullptr;
        Property* Member = nullptr;
        nlohmann::json Value;
    };

    static nlohmann::json Blend(Property& InMember, const nlohmann::json& InFrom, const nlohmann::json& InTo, float InAlpha);

    void Set(Node& InTarget, Property& InMember, nlohmann::json InValue);
    void BlendTowards(const AnimationPose& InOther, float InAlpha);
    void Apply() const;
    void Clear() { m_Channels.Clear(); }

    const Array<Channel>& GetChannels() const { return m_Channels; }

private:
    Channel* Find(const Node& InTarget, const Property& InMember);

    Array<Channel> m_Channels;
};
