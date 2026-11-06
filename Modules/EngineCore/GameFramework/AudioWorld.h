#pragma once
#include "WorldSubsystem.h"
#include "Audio/AudioEngine.h"
#include "Object/Pointer.h"
#include "AudioWorld.gen.h"

class AudioListenerNode;
class Node3D;

class AudioWorld : public WorldSubsystem {
public:
    ARTIFACT_CLASS();

    virtual void WorldUpdate(float InDeltaTime) override;
    virtual void Shutdown() override;

    AudioHandle Play(AudioClip* InClip, const AudioPlayParams& InParams = {});
    AudioHandle PlayOneShot(AudioClip* InClip, float InVolume = 1.0f, float InPitch = 1.0f, const String& InBus = "Master");
    AudioHandle PlayOneShotAtLocation(AudioClip* InClip, const Vec3& InPosition, float InVolume = 1.0f, float InPitch = 1.0f,
                                      const AudioAttenuation& InAttenuation = {}, const String& InBus = "Master");
    void StopAll(float InFadeOutTime = 0.0f);

    AudioListenerNode* GetListener() const { return m_Listener.Get(); }
    void SetListener(AudioListenerNode* InListener);

private:
    WeakObjectPtr<AudioListenerNode> m_Listener;
    const Node3D* m_LastListenerNode = nullptr;
    Vec3 m_LastListenerPosition = Vec3(0.0f);
    bool m_HasListenerPosition = false;
};
