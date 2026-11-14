#pragma once
#include "NodeAsset.h"
#include "Common/Array.h"
#include "Animation.gen.h"

class Node;
class AnimationPose;
struct Property;

ARTIFACT_ENUM();
enum class AnimationInterpolation : uint8_t {
    None = 0,
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

/** One property edit at one frame. */
struct AnimationKey {
    ARTIFACT_STRUCT();

    PROPERTY()
    Array<int32_t> Path;

    PROPERTY()
    String PropertyName;

    PROPERTY()
    int32_t Frame = 0;

    PROPERTY()
    AnimationInterpolation Interpolation = AnimationInterpolation::Linear;

    PROPERTY()
    String Value;
};

struct AnimationEvent {
    ARTIFACT_STRUCT();

    PROPERTY()
    int32_t Frame = 0;

    PROPERTY()
    String Name;
};

/** Every key of one property in frame order. */
struct AnimationTrack {
    Array<int32_t> Path;
    String PropertyName;
    Array<int32_t> Keys;
    bool Warned = false;
};

/** A list of keyframes over a node hierarchy. */
class Animation : public NodeAsset {
public:
    ARTIFACT_CLASS();

    static Animation* CreateEmpty(const String& InDirectory, const String& InName, const Class& InPlaceholderRoot);

    void ApplyTo(Node* InRootNode, float InTime);
    void ApplyAtFrame(Node* InRootNode, float InFrame);
    void Sample(Node& InRootNode, float InFrame, AnimationPose& OutPose);
    float FrameAtTime(float InTime) const;

    FUNCTION(Pure)
    float GetFrameRate() const { return m_FrameRate; }
    void SetFrameRate(float InFrameRate);
    FUNCTION(Pure)
    int32_t GetFrameCount() const { return m_FrameCount; }
    void SetFrameCount(int32_t InFrameCount);
    FUNCTION(Pure)
    float GetDuration() const;

    FUNCTION(Pure)
    bool IsLooping() const { return m_Loop; }
    FUNCTION()
    void SetLooping(bool InLooping) { m_Loop = InLooping; }

    const Array<AnimationEvent>& GetEvents() const { return m_Events; }
    int32_t FindEvent(int32_t InFrame) const;
    int32_t AddEvent(int32_t InFrame, const String& InName);
    void SetEventFrame(int32_t InEvent, int32_t InFrame);
    void SetEventName(int32_t InEvent, const String& InName);
    void RemoveEvent(int32_t InEvent);
    void CollectEventsBetween(float InFromFrame, float InToFrame, Array<String>& OutNames) const;

    const Array<AnimationKey>& GetKeys() const { return m_Keys; }
    const Array<AnimationTrack>& GetTracks() const { return m_Tracks; }

    int32_t FindKey(const Array<int32_t>& InPath, const String& InPropertyName, int32_t InFrame) const;
    int32_t FindTrack(const Array<int32_t>& InPath, const String& InPropertyName) const;
    void SetKey(const Array<int32_t>& InPath, const String& InPropertyName, int32_t InFrame, const String& InValue);
    /** Keys may share a frame with others of their track until MergeKeys settles the move. */
    void MoveKeys(const Array<int32_t>& InKeys, int32_t InFrame);
    /** InKeys win over every other key of their track on the same frame; returns their new indices. */
    Array<int32_t> MergeKeys(const Array<int32_t>& InKeys);
    void SetKeyInterpolation(int32_t InKey, AnimationInterpolation InInterpolation);
    void RemoveKey(int32_t InKey);
    void RemoveTrack(const Array<int32_t>& InPath, const String& InPropertyName);

    /** Captures what InNode currently holds in InPropertyName as a key at InFrame. */
    bool KeyProperty(Node& InRoot, Node& InNode, const String& InPropertyName, int32_t InFrame);

    static Node* ResolvePath(Node& InRoot, const Array<int32_t>& InPath);
    static bool MakePath(Node& InRoot, Node& InTarget, Array<int32_t>& OutPath);
    static bool IsAnimatable(Property* InProperty);

    const Array<int32_t>& GetRootPath() const { return m_RootPath; }
    void SetRootPath(const Array<int32_t>& InPath) { m_RootPath = InPath; }

    virtual String SerializeToJson() const override;
    virtual void DeserializeFromJson(const String& InJson) override;

protected:
    virtual void Load() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

    PROPERTY(Range = (1, 240))
    float m_FrameRate = 30.0f;

    PROPERTY(Range = (1, 100000))
    int32_t m_FrameCount = 60;

    PROPERTY()
    bool m_Loop = false;

    PROPERTY()
    Array<AnimationKey> m_Keys;

    PROPERTY()
    Array<AnimationEvent> m_Events;

private:
    void RebuildTracks();
    void SampleTrack(AnimationTrack& OutTrack, Node& InRoot, float InFrame, AnimationPose& OutPose);

    Array<AnimationTrack> m_Tracks;
    Array<int32_t> m_RootPath;
};
