#include "Animation.h"
#include "AssetManager.h"
#include "NodeRecord.h"
#include "GameFramework/Node.h"
#include "Object/Property.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include "Core/Log.h"
#include <cmath>

using json = nlohmann::json;

static const char* s_RotationAxes[] = { "X", "Y", "Z", "W" };

static String PathToString(const Array<int32_t>& InPath) {
    if (InPath.IsEmpty()) {
        return "root";
    }
    String text;
    for (int32_t index : InPath) {
        text += (text.empty() ? "" : "/") + std::to_string(index);
    }
    return text;
}

static float Ease(AnimationInterpolation InInterpolation, float InAlpha) {
    switch (InInterpolation) {
        case AnimationInterpolation::EaseIn:    return InAlpha * InAlpha;
        case AnimationInterpolation::EaseOut:   return 1.0f - (1.0f - InAlpha) * (1.0f - InAlpha);
        case AnimationInterpolation::EaseInOut: return InAlpha * InAlpha * (3.0f - 2.0f * InAlpha);
        default:                                return InAlpha;
    }
}

/** Numbers blend, everything else holds the earlier value until the next key takes over. */
static json Blend(const json& InA, const json& InB, float InAlpha) {
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
                result[name] = Blend(value, InB[name], InAlpha);
            }
        }
        return result;
    }
    if (InA.is_array() && InB.is_array() && InA.size() == InB.size()) {
        json result = InA;
        for (size_t i = 0; i < InA.size(); i++) {
            result[i] = Blend(InA[i], InB[i], InAlpha);
        }
        return result;
    }
    return InA;
}

static json BlendRotation(const json& InA, const json& InB, float InAlpha) {
    double from[4], to[4], dot = 0.0;
    for (int32_t i = 0; i < 4; i++) {
        if (!InA[s_RotationAxes[i]].is_number() || !InB[s_RotationAxes[i]].is_number()) {
            return Blend(InA, InB, InAlpha);
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

static bool IsRotation(Property* InProperty) {
    StructProperty* structProperty = Cast<StructProperty>(InProperty);
    return structProperty && structProperty->InnerStructTypename == "Quat";
}

Animation* Animation::CreateEmpty(const String& InDirectory, const String& InName, const Class& InPlaceholderRoot) {
    Animation* animation = Cast<Animation>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!animation) {
        return nullptr;
    }

    NodeRecord* record = new NodeRecord();
    record->ClassName = InPlaceholderRoot.Name;
    animation->SetRoot(SharedObjectPtr<NodeRecord>(record));
    AssetManager::Get().SaveAsset(animation);
    return animation;
}

Node* Animation::ResolvePath(Node& InRoot, const Array<int32_t>& InPath) {
    Node* node = &InRoot;
    for (int32_t index : InPath) {
        if (index < 0 || index >= (int32_t)node->GetChildCount()) {
            return nullptr;
        }
        node = node->GetChild(index);
    }
    return node;
}

bool Animation::MakePath(Node& InRoot, Node& InTarget, Array<int32_t>& OutPath) {
    OutPath.Clear();
    for (Node* node = &InTarget; node != &InRoot; node = node->GetParent()) {
        if (!node->GetParent()) {
            OutPath.Clear();
            return false;
        }
        OutPath.Insert(0, node->GetSiblingIndex());
    }
    return true;
}

bool Animation::IsAnimatable(Property* InProperty) {
    return InProperty && !Cast<SharedObjectPtrProperty>(InProperty);
}

float Animation::GetDuration() const {
    return m_FrameRate > 0.0f ? (float)m_FrameCount / m_FrameRate : 0.0f;
}

void Animation::SetFrameRate(float InFrameRate) {
    m_FrameRate = InFrameRate > 1.0f ? InFrameRate : 1.0f;
}

void Animation::SetFrameCount(int32_t InFrameCount) {
    m_FrameCount = InFrameCount > 1 ? InFrameCount : 1;
}

float Animation::FrameAtTime(float InTime) const {
    const float last = (float)m_FrameCount;
    const float frame = InTime * m_FrameRate;
    if (!m_Loop) {
        return frame < 0.0f ? 0.0f : (frame > last ? last : frame);
    }
    return frame - std::floor(frame / last) * last;
}

int32_t Animation::FindKey(const Array<int32_t>& InPath, const String& InPropertyName, int32_t InFrame) const {
    for (int32_t i = 0; i < m_Keys.Size(); i++) {
        const AnimationKey& key = m_Keys[i];
        if (key.Frame == InFrame && key.PropertyName == InPropertyName && key.Path == InPath) {
            return i;
        }
    }
    return -1;
}

int32_t Animation::FindTrack(const Array<int32_t>& InPath, const String& InPropertyName) const {
    for (int32_t i = 0; i < m_Tracks.Size(); i++) {
        if (m_Tracks[i].PropertyName == InPropertyName && m_Tracks[i].Path == InPath) {
            return i;
        }
    }
    return -1;
}

void Animation::SetKey(const Array<int32_t>& InPath, const String& InPropertyName, int32_t InFrame, const String& InValue) {
    const int32_t existing = FindKey(InPath, InPropertyName, InFrame);
    if (existing >= 0) {
        m_Keys[existing].Value = InValue;
        return;
    }

    AnimationKey key;
    key.Path = InPath;
    key.PropertyName = InPropertyName;
    key.Frame = InFrame;
    key.Value = InValue;
    m_Keys.Add(key);
    RebuildTracks();
}

int32_t Animation::SetKeyFrame(int32_t InKey, int32_t InFrame) {
    if (InKey < 0 || InKey >= m_Keys.Size() || m_Keys[InKey].Frame == InFrame) {
        return InKey;
    }

    const int32_t occupied = FindKey(m_Keys[InKey].Path, m_Keys[InKey].PropertyName, InFrame);
    m_Keys[InKey].Frame = InFrame;
    if (occupied < 0) {
        RebuildTracks();
        return InKey;
    }

    m_Keys.RemoveAt(occupied);
    RebuildTracks();
    return occupied < InKey ? InKey - 1 : InKey;
}

void Animation::SetKeyInterpolation(int32_t InKey, AnimationInterpolation InInterpolation) {
    if (InKey >= 0 && InKey < m_Keys.Size()) {
        m_Keys[InKey].Interpolation = InInterpolation;
    }
}

void Animation::RemoveKey(int32_t InKey) {
    if (InKey >= 0 && InKey < m_Keys.Size()) {
        m_Keys.RemoveAt(InKey);
        RebuildTracks();
    }
}

void Animation::RemoveTrack(const Array<int32_t>& InPath, const String& InPropertyName) {
    for (int32_t i = m_Keys.Size() - 1; i >= 0; i--) {
        if (m_Keys[i].PropertyName == InPropertyName && m_Keys[i].Path == InPath) {
            m_Keys.RemoveAt(i);
        }
    }
    RebuildTracks();
}

bool Animation::KeyProperty(Node& InRoot, Node& InNode, const String& InPropertyName, int32_t InFrame) {
    Array<int32_t> path;
    if (!MakePath(InRoot, InNode, path)) {
        AE_WARN("'{0}' is not below the animated root; no key was recorded", InNode.GetName());
        return false;
    }

    Property* property = Property::FindTypeProperty(InNode.GetClass(), InPropertyName);
    if (!IsAnimatable(property)) {
        AE_WARN("'{0}' of '{1}' cannot be animated", InPropertyName, InNode.GetClass().Name);
        return false;
    }

    SetKey(path, InPropertyName, InFrame,
           JsonSerializer::SerializeProperty(property, property->GetValuePtr(&InNode)).dump());
    return true;
}

void Animation::RebuildTracks() {
    m_Tracks.Clear();
    for (int32_t i = 0; i < m_Keys.Size(); i++) {
        const AnimationKey& key = m_Keys[i];

        int32_t track = -1;
        for (int32_t candidate = 0; candidate < m_Tracks.Size(); candidate++) {
            if (m_Tracks[candidate].PropertyName == key.PropertyName && m_Tracks[candidate].Path == key.Path) {
                track = candidate;
                break;
            }
        }
        if (track < 0) {
            AnimationTrack added;
            added.Path = key.Path;
            added.PropertyName = key.PropertyName;
            m_Tracks.Add(added);
            track = m_Tracks.Last();
        }
        m_Tracks[track].Keys.Add(i);
    }

    for (AnimationTrack& track : m_Tracks) {
        track.Keys.Sort([this](const int32_t& InA, const int32_t& InB) { return m_Keys[InA].Frame < m_Keys[InB].Frame; });
    }
}

void Animation::ApplyTo(Node* InRootNode, float InTime) {
    ApplyAtFrame(InRootNode, FrameAtTime(InTime));
}

void Animation::ApplyAtFrame(Node* InRootNode, float InFrame) {
    if (!InRootNode) {
        return;
    }
    const float last = (float)m_FrameCount;
    const float frame = InFrame < 0.0f ? 0.0f : (InFrame > last ? last : InFrame);
    for (AnimationTrack& track : m_Tracks) {
        ApplyTrack(track, *InRootNode, frame);
    }
}

void Animation::ApplyTrack(AnimationTrack& OutTrack, Node& InRoot, float InFrame) {
    // A hierarchy that no longer matches is reported once, not once per frame.
    const auto warn = [this, &OutTrack](const String& InReason) {
        if (!OutTrack.Warned) {
            OutTrack.Warned = true;
            AE_WARN("Animation '{0}': {1} ({2} -> {3})", GetDisplayName(), InReason,
                    PathToString(OutTrack.Path), OutTrack.PropertyName);
        }
    };

    if (OutTrack.Keys.IsEmpty()) {
        return;
    }

    Node* node = ResolvePath(InRoot, OutTrack.Path);
    if (!node) {
        warn("no node at this path");
        return;
    }

    Property* property = Property::FindTypeProperty(node->GetClass(), OutTrack.PropertyName);
    if (!property) {
        warn("'" + node->GetClass().Name + "' has no such property");
        return;
    }

    const AnimationKey* previous = nullptr;
    const AnimationKey* next = nullptr;
    for (int32_t index : OutTrack.Keys) {
        const AnimationKey& key = m_Keys[index];
        if ((float)key.Frame <= InFrame) {
            previous = &key;
        } else {
            next = &key;
            break;
        }
    }

    json value;
    if (!previous) {
        value = json::parse(next->Value, nullptr, false);
    } else if (!next || previous->Interpolation == AnimationInterpolation::None) {
        value = json::parse(previous->Value, nullptr, false);
    } else {
        const json from = json::parse(previous->Value, nullptr, false);
        const json to = json::parse(next->Value, nullptr, false);
        const float span = (float)(next->Frame - previous->Frame);
        const float alpha = Ease(previous->Interpolation, span > 0.0f ? (InFrame - (float)previous->Frame) / span : 1.0f);
        value = (IsRotation(property) && from.is_object() && to.is_object()) ? BlendRotation(from, to, alpha)
                                                                            : Blend(from, to, alpha);
    }

    if (value.is_discarded()) {
        warn("the stored value is not valid JSON");
        return;
    }

    try {
        JsonSerializer::DeserializeProperty(property, property->GetValuePtr(node), value);
    } catch (const std::exception&) {
        warn("the stored value does not fit the property's type");
        return;
    }
    property->NotifyChanged(node);
}

String Animation::SerializeToJson() const {
    json asset = json::parse(Super::SerializeToJson());
    asset["RootPath"] = m_RootPath.GetData();
    return asset.dump(4);
}

void Animation::DeserializeFromJson(const String& InJson) {
    Super::DeserializeFromJson(InJson);

    m_RootPath.Clear();
    const json asset = json::parse(InJson, nullptr, false);
    if (!asset.is_discarded() && asset.contains("RootPath") && asset["RootPath"].is_array()) {
        for (const json& index : asset["RootPath"]) {
            m_RootPath.Add(index.get<int32_t>());
        }
    }
    RebuildTracks();
}

void Animation::Load() {
    Super::Load();
    RebuildTracks();
}

void Animation::Cook(ChunkedBinary& OutChunkedBinary) {
    // Deliberately not NodeAsset::Cook: the placeholder hierarchy never ships.
    Asset::Cook(OutChunkedBinary);
}
