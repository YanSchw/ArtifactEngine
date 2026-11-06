#include "AudioWorld.h"
#include "AudioListenerNode.h"
#include "CameraNode.h"
#include "World.h"

void AudioWorld::WorldUpdate(float InDeltaTime) {
    if (!AudioEngine::IsAvailable()) {
        return;
    }

    AudioListenerNode* listenerNode = m_Listener.Get();
    Node3D* listener = listenerNode && listenerNode->IsEnabled() ? (Node3D*)listenerNode : (Node3D*)GetWorld().GetMainCamera();
    if (!listener) {
        return;
    }

    const Vec3 position = listener->GetPosition();
    const bool continuous = m_HasListenerPosition && listener == m_LastListenerNode && InDeltaTime > 0.0f;
    const Vec3 velocity = continuous ? (position - m_LastListenerPosition) / InDeltaTime : Vec3(0.0f);
    m_LastListenerPosition = position;
    m_LastListenerNode = listener;
    m_HasListenerPosition = true;

    AudioEngine::Get().SetListener(AudioListener{ position, listener->GetRotation(), velocity });
}

void AudioWorld::Shutdown() {
    StopAll();
}

AudioHandle AudioWorld::Play(AudioClip* InClip, const AudioPlayParams& InParams) {
    if (!AudioEngine::IsAvailable()) {
        return {};
    }
    AudioPlayParams params = InParams;
    params.OwningWorld = &GetWorld();
    return AudioEngine::Get().Play(InClip, params);
}

AudioHandle AudioWorld::PlayOneShot(AudioClip* InClip, float InVolume, float InPitch, const String& InBus) {
    AudioPlayParams params;
    params.Volume = InVolume;
    params.Pitch = InPitch;
    params.Bus = InBus;
    return Play(InClip, params);
}

AudioHandle AudioWorld::PlayOneShotAtLocation(AudioClip* InClip, const Vec3& InPosition, float InVolume, float InPitch,
                                              const AudioAttenuation& InAttenuation, const String& InBus) {
    AudioPlayParams params;
    params.Volume = InVolume;
    params.Pitch = InPitch;
    params.Bus = InBus;
    params.Spatial = true;
    params.Position = InPosition;
    params.Attenuation = InAttenuation;
    return Play(InClip, params);
}

void AudioWorld::StopAll(float InFadeOutTime) {
    if (AudioEngine::IsAvailable()) {
        AudioEngine::Get().StopAll(&GetWorld(), InFadeOutTime);
    }
}

void AudioWorld::SetListener(AudioListenerNode* InListener) {
    m_Listener = InListener;
}
