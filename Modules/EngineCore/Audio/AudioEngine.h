#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Audio/AudioPlayParams.h"
#include "Audio/AudioSampleBuffer.h"
#include <functional>
#include <mutex>
#include "AudioEngine.gen.h"

class AudioClip;
class AudioDevice;
struct AudioVoice;

struct AudioHandle {
    uint64_t Id = 0;

    bool IsValid() const { return Id != 0; }
    bool IsPlaying() const;
    bool IsPaused() const;

    void Stop(float InFadeOutTime = 0.0f) const;
    void SetPaused(bool InPaused) const;

    void SetVolume(float InVolume) const;
    void SetPitch(float InPitch) const;
    void SetPan(float InPan) const;
    void SetLooping(bool InLoop) const;
    void SetPosition(const Vec3& InPosition, const Vec3& InVelocity = Vec3(0.0f)) const;
    bool ApplyParams(const AudioPlayParams& InParams) const;

    double GetTime() const;
    void SetTime(double InSeconds) const;

private:
    bool Visit(const std::function<void(AudioVoice&)>& InVisitor) const;
};

struct AudioVoice {
    uint64_t Id = 0;
    SharedAudioSamples Samples;
    World* OwningWorld = nullptr;
    int32_t Bus = 0;

    double Cursor = 0.0;
    float Volume = 1.0f;
    float Pitch = 1.0f;
    float Pan = 0.0f;
    float Fade = 1.0f;
    float FadeRate = 0.0f;
    bool Loop = false;
    bool Paused = false;
    bool Stopping = false;
    bool Finished = false;
    bool Started = false;

    bool Spatial = false;
    Vec3 Position = Vec3(0.0f);
    Vec3 Velocity = Vec3(0.0f);
    AudioAttenuation Attenuation;

    float Gains[2][2] = {};
};

struct AudioBus {
    String Name;
    int32_t Parent = -1;
    float Volume = 1.0f;
    bool Muted = false;
    bool Paused = false;
};

struct AudioListener {
    Vec3 Position = Vec3(0.0f);
    Quat Rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 Velocity = Vec3(0.0f);
};

class AudioEngine : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~AudioEngine();

    static AudioEngine& Get();
    static bool IsAvailable();

    void Initialize(bool InOpenDevice = true);
    void Shutdown();
    void Update();

    void Render(const AudioFormat& InFormat, float* OutSamples, uint32_t InFrameCount);

    AudioHandle Play(AudioClip* InClip, const AudioPlayParams& InParams = {});
    AudioHandle Play(const SharedAudioSamples& InSamples, const AudioPlayParams& InParams = {});
    void StopAll(float InFadeOutTime = 0.0f);
    void StopAll(World* InOwningWorld, float InFadeOutTime = 0.0f);

    AudioListener GetListener() const;
    void SetListener(const AudioListener& InListener);

    void CreateBus(const String& InName, const String& InParent = "Master");
    float GetBusVolume(const String& InBus) const;
    void SetBusVolume(const String& InBus, float InVolume);
    bool IsBusMuted(const String& InBus) const;
    void SetBusMuted(const String& InBus, bool InMuted);
    bool IsBusPaused(const String& InBus) const;
    void SetBusPaused(const String& InBus, bool InPaused);

    float GetDopplerFactor() const { return m_DopplerFactor; }
    void SetDopplerFactor(float InFactor) { m_DopplerFactor = InFactor; }
    float GetSpeedOfSound() const { return m_SpeedOfSound; }
    void SetSpeedOfSound(float InSpeed) { m_SpeedOfSound = InSpeed; }

    AudioDevice* GetDevice() const { return m_Device.Get(); }
    int32_t GetPlayingVoiceCount() const;

private:
    AudioVoice* FindVoice(uint64_t InId);
    bool VisitVoice(uint64_t InId, const std::function<void(AudioVoice&)>& InVisitor);
    void StopVoice(AudioVoice& OutVoice, float InFadeOutTime);
    void StealVoice();
    void RemoveFinishedVoices();

    int32_t FindBus(const String& InName) const;
    int32_t FindOrAddBus(const String& InName);
    float GetBusGain(int32_t InBus) const;
    bool IsBusPausedInHierarchy(int32_t InBus) const;

    float ComputeDoppler(const AudioVoice& InVoice) const;
    void ComputeGains(const AudioVoice& InVoice, uint32_t InSourceChannels, uint32_t InOutputChannels, float OutGains[2][2]) const;
    void MixVoice(AudioVoice& OutVoice, const AudioFormat& InFormat, float* OutSamples, uint32_t InFrameCount);

    mutable std::mutex m_Mutex;
    Array<AudioVoice> m_Voices;
    Array<AudioBus> m_Buses;
    AudioListener m_Listener;
    uint64_t m_NextVoiceId = 1;
    int32_t m_MaxVoices = 128;
    float m_DopplerFactor = 1.0f;
    float m_SpeedOfSound = 343.0f;
    SharedObjectPtr<AudioDevice> m_Device;

    friend struct AudioHandle;
};
