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

    void Play(float InFadeInTime = 0.0f);
    void Stop(float InFadeOutTime = 0.0f);
    void SetPaused(bool InPaused);
    bool IsPlaying() const;
    bool IsPaused() const;

    AudioHandle PlayOneShot(AudioClip* InClip = nullptr, float InVolumeScale = 1.0f);

    double GetPlaybackTime() const;
    void SetPlaybackTime(double InSeconds);

    AudioClip* GetClip() const;
    void SetClip(AudioClip* InClip);

    bool GetAutoPlay() const { return m_AutoPlay; }
    void SetAutoPlay(bool InAutoPlay) { m_AutoPlay = InAutoPlay; }
    bool IsLooping() const { return m_Loop; }
    void SetLooping(bool InLoop) { m_Loop = InLoop; }
    float GetVolume() const { return m_Volume; }
    void SetVolume(float InVolume) { m_Volume = InVolume; }
    float GetPitch() const { return m_Pitch; }
    void SetPitch(float InPitch) { m_Pitch = InPitch; }
    float GetPan() const { return m_Pan; }
    void SetPan(float InPan) { m_Pan = InPan; }
    bool IsSpatial() const { return m_Spatial; }
    void SetSpatial(bool InSpatial) { m_Spatial = InSpatial; }
    const AudioAttenuation& GetAttenuation() const { return m_Attenuation; }
    void SetAttenuation(const AudioAttenuation& InAttenuation) { m_Attenuation = InAttenuation; }
    const String& GetBus() const { return m_Bus; }
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

    PROPERTY()
    float m_Volume = 1.0f;

    PROPERTY()
    float m_Pitch = 1.0f;

    PROPERTY()
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
