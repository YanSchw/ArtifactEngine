#pragma once
#include "Node3D.h"
#include "Audio/AudioEngine.h"
#include "Object/Pointer.h"
#include "AudioSourceNode.gen.h"

class AudioClip;
class AudioWorld;

struct AudioSourceLayer {
    AudioHandle Handle;
    float VolumeScale = 1.0f;
};

class AudioSourceNode : public Node3D {
public:
    ARTIFACT_CLASS();

    virtual void BeginPlay() override;
    virtual void EndPlay() override;
    virtual void WorldUpdate(float InDeltaTime) override;

    FUNCTION()
    void Play(float InFadeInTime = 0.0f);
    FUNCTION()
    void Stop(float InFadeOutTime = 0.0f);
    FUNCTION()
    void SetPaused(bool InPaused);
    FUNCTION(Pure)
    bool IsPlaying() const;
    FUNCTION(Pure)
    bool IsPaused() const;

    AudioHandle PlayOneShot(AudioClip* InClip = nullptr, float InVolumeScale = 1.0f);

    FUNCTION(Pure)
    double GetPlaybackTime() const;
    FUNCTION()
    void SetPlaybackTime(double InSeconds);

    FUNCTION(Pure)
    AudioClip* GetClip() const;
    FUNCTION()
    void SetClip(AudioClip* InClip);

    FUNCTION(Pure)
    bool GetAutoPlay() const { return m_AutoPlay; }
    FUNCTION()
    void SetAutoPlay(bool InAutoPlay) { m_AutoPlay = InAutoPlay; }
    FUNCTION(Pure)
    bool IsLooping() const { return m_Loop; }
    FUNCTION()
    void SetLooping(bool InLoop) { m_Loop = InLoop; }
    FUNCTION(Pure)
    float GetVolume() const { return m_Volume; }
    FUNCTION()
    void SetVolume(float InVolume) { m_Volume = InVolume; }
    FUNCTION(Pure)
    float GetPitch() const { return m_Pitch; }
    FUNCTION()
    void SetPitch(float InPitch) { m_Pitch = InPitch; }
    FUNCTION(Pure)
    float GetPan() const { return m_Pan; }
    FUNCTION()
    void SetPan(float InPan) { m_Pan = InPan; }
    FUNCTION(Pure)
    bool IsSpatial() const { return m_Spatial; }
    FUNCTION()
    void SetSpatial(bool InSpatial) { m_Spatial = InSpatial; }
    FUNCTION(Pure)
    const AudioAttenuation& GetAttenuation() const { return m_Attenuation; }
    FUNCTION()
    void SetAttenuation(const AudioAttenuation& InAttenuation) { m_Attenuation = InAttenuation; }
    FUNCTION(Pure)
    const String& GetBus() const { return m_Bus; }
    FUNCTION()
    void SetBus(const String& InBus) { m_Bus = InBus; }

private:
    AudioWorld* GetAudioWorld() const;
    AudioPlayParams MakeParams(float InVolumeScale, bool InLoop) const;

    PROPERTY()
    WeakObjectPtr<AudioClip> m_Clip;

    PROPERTY()
    bool m_AutoPlay = true;

    PROPERTY()
    bool m_Loop = false;

    PROPERTY(Range = (0, 1))
    float m_Volume = 1.0f;

    PROPERTY(Range = (0.01, 4))
    float m_Pitch = 1.0f;

    PROPERTY(Range = (-1, 1))
    float m_Pan = 0.0f;

    PROPERTY()
    bool m_Spatial = true;

    PROPERTY()
    AudioAttenuation m_Attenuation;

    PROPERTY()
    String m_Bus = "Master";

    AudioHandle m_Voice;
    Array<AudioSourceLayer> m_Layers;
    Vec3 m_LastPosition = Vec3(0.0f);
    Vec3 m_Velocity = Vec3(0.0f);
};
