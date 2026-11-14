#include "Animation.h"
#include "AssetManager.h"
#include "Animation/AnimationPose.h"
#include "NodeRecord.h"
#include "GameFramework/Node.h"
#include "Object/Property.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include "Core/Log.h"
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

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
    return InProperty && !Cast<SharedObjectPtrProperty>(InProperty) && !InProperty->IsTransient();
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

void Animation::MoveKeys(const Array<int32_t>& InKeys, int32_t InFrame) {
    for (int32_t key : InKeys) {
        if (key >= 0 && key < m_Keys.Size()) {
            m_Keys[key].Frame = std::clamp(InFrame, 0, m_FrameCount);
        }
    }
    RebuildTracks();
}

Array<int32_t> Animation::MergeKeys(const Array<int32_t>& InKeys) {
    const auto covered = [this, &InKeys](const AnimationKey& InKey) {
        for (int32_t winner : InKeys) {
            const AnimationKey& other = m_Keys[winner];
            if (other.Frame == InKey.Frame && other.PropertyName == InKey.PropertyName && other.Path == InKey.Path) {
                return true;
            }
        }
        return false;
    };

    Array<int32_t> remapped;
    for (int32_t i = 0; i < InKeys.Size(); i++) {
        remapped.Add(-1);
    }
    Array<AnimationKey> kept;
    for (int32_t i = 0; i < m_Keys.Size(); i++) {
        const int32_t moved = InKeys.IndexOf(i);
        if (moved < 0 && covered(m_Keys[i])) {
            continue;
        }
        if (moved >= 0) {
            remapped[moved] = kept.Size();
        }
        kept.Add(m_Keys[i]);
    }
    m_Keys = kept;
    RebuildTracks();
    return remapped;
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
    AnimationPose pose;
    Sample(*InRootNode, InFrame, pose);
    pose.Apply();
}

void Animation::Sample(Node& InRootNode, float InFrame, AnimationPose& OutPose) {
    const float frame = std::clamp(InFrame, 0.0f, (float)m_FrameCount);
    for (AnimationTrack& track : m_Tracks) {
        SampleTrack(track, InRootNode, frame, OutPose);
    }
}

void Animation::SampleTrack(AnimationTrack& OutTrack, Node& InRoot, float InFrame, AnimationPose& OutPose) {
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
        value = AnimationPose::Blend(*property, from, to, alpha);
    }

    if (value.is_discarded()) {
        warn("the stored value is not valid JSON");
        return;
    }
    OutPose.Set(*node, *property, std::move(value));
}

int32_t Animation::FindEvent(int32_t InFrame) const {
    for (int32_t i = 0; i < m_Events.Size(); i++) {
        if (m_Events[i].Frame == InFrame) {
            return i;
        }
    }
    return -1;
}

int32_t Animation::AddEvent(int32_t InFrame, const String& InName) {
    const int32_t existing = FindEvent(InFrame);
    if (existing >= 0) {
        return existing;
    }
    AnimationEvent event;
    event.Frame = InFrame;
    event.Name = InName;
    m_Events.Add(event);
    return m_Events.Last();
}

void Animation::SetEventFrame(int32_t InEvent, int32_t InFrame) {
    const int32_t frame = std::clamp(InFrame, 0, m_FrameCount);
    if (InEvent >= 0 && InEvent < m_Events.Size() && FindEvent(frame) < 0) {
        m_Events[InEvent].Frame = frame;
    }
}

void Animation::SetEventName(int32_t InEvent, const String& InName) {
    if (InEvent >= 0 && InEvent < m_Events.Size()) {
        m_Events[InEvent].Name = InName;
    }
}

void Animation::RemoveEvent(int32_t InEvent) {
    if (InEvent >= 0 && InEvent < m_Events.Size()) {
        m_Events.RemoveAt(InEvent);
    }
}

void Animation::CollectEventsBetween(float InFromFrame, float InToFrame, Array<String>& OutNames) const {
    if (InToFrame <= InFromFrame) {
        return;
    }
    const float length = (float)m_FrameCount;
    const float from = m_Loop ? std::max(InFromFrame, InToFrame - length) : InFromFrame;
    const int64_t firstCycle = m_Loop ? std::max<int64_t>(0, (int64_t)std::floor(from / length)) : 0;
    const int64_t lastCycle = m_Loop ? (int64_t)std::floor(InToFrame / length) : 0;

    Array<std::pair<float, String>> passed;
    for (int64_t cycle = firstCycle; cycle <= lastCycle; cycle++) {
        for (const AnimationEvent& event : m_Events) {
            const float position = (float)cycle * length + (float)event.Frame;
            if (position > from && position <= InToFrame) {
                passed.Add({ position, event.Name });
            }
        }
    }
    passed.Sort([](const auto& InA, const auto& InB) { return InA.first < InB.first; });
    for (const auto& [position, name] : passed) {
        OutNames.Add(name);
    }
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
