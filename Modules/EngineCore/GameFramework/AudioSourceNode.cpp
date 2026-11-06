#include "AudioSourceNode.h"
#include "AudioWorld.h"
#include "World.h"
#include "Assets/AudioClip.h"

void AudioSourceNode::BeginPlay() {
    Super::BeginPlay();
    SetUpdateFlag(UpdateFlag::WorldUpdate);
    m_LastPosition = GetPosition();
    if (m_AutoPlay) {
        Play();
    }
}

void AudioSourceNode::EndPlay() {
    Stop();
    for (const AudioSourceLayer& layer : m_Layers) {
        layer.Handle.Stop();
    }
    m_Layers.Clear();
    Super::EndPlay();
}

void AudioSourceNode::WorldUpdate(float InDeltaTime) {
    Super::WorldUpdate(InDeltaTime);

    const Vec3 position = GetPosition();
    m_Velocity = InDeltaTime > 0.0f ? (position - m_LastPosition) / InDeltaTime : Vec3(0.0f);
    m_LastPosition = position;

    if (m_Voice.IsValid() && !m_Voice.ApplyParams(MakeParams(1.0f, m_Loop))) {
        m_Voice = AudioHandle();
    }
    m_Layers.Filter([this](const AudioSourceLayer& InLayer) {
        return InLayer.Handle.ApplyParams(MakeParams(InLayer.VolumeScale, false));
    });
}

void AudioSourceNode::Play(float InFadeInTime) {
    m_Voice.Stop();
    m_Voice = AudioHandle();

    AudioWorld* audio = GetAudioWorld();
    if (!audio || !m_Clip.Get()) {
        return;
    }
    AudioPlayParams params = MakeParams(1.0f, m_Loop);
    params.FadeInTime = InFadeInTime;
    m_Voice = audio->Play(m_Clip.Get(), params);
}

void AudioSourceNode::Stop(float InFadeOutTime) {
    m_Voice.Stop(InFadeOutTime);
    m_Voice = AudioHandle();
}

void AudioSourceNode::SetPaused(bool InPaused) {
    m_Voice.SetPaused(InPaused);
    for (const AudioSourceLayer& layer : m_Layers) {
        layer.Handle.SetPaused(InPaused);
    }
}

bool AudioSourceNode::IsPlaying() const {
    return m_Voice.IsPlaying();
}

bool AudioSourceNode::IsPaused() const {
    return m_Voice.IsPaused();
}

AudioHandle AudioSourceNode::PlayOneShot(AudioClip* InClip, float InVolumeScale) {
    AudioWorld* audio = GetAudioWorld();
    AudioClip* clip = InClip ? InClip : m_Clip.Get();
    if (!audio || !clip) {
        return {};
    }
    const AudioHandle handle = audio->Play(clip, MakeParams(InVolumeScale, false));
    if (handle.IsValid()) {
        m_Layers.Add(AudioSourceLayer{ handle, InVolumeScale });
    }
    return handle;
}

double AudioSourceNode::GetPlaybackTime() const {
    return m_Voice.GetTime();
}

void AudioSourceNode::SetPlaybackTime(double InSeconds) {
    m_Voice.SetTime(InSeconds);
}

AudioClip* AudioSourceNode::GetClip() const {
    return m_Clip.Get();
}

void AudioSourceNode::SetClip(AudioClip* InClip) {
    m_Clip = InClip;
}

AudioWorld* AudioSourceNode::GetAudioWorld() const {
    World* world = GetWorld();
    return world ? world->GetSubsystem<AudioWorld>() : nullptr;
}

AudioPlayParams AudioSourceNode::MakeParams(float InVolumeScale, bool InLoop) const {
    AudioPlayParams params;
    params.Volume = m_Volume * InVolumeScale;
    params.Pitch = m_Pitch;
    params.Pan = m_Pan;
    params.Loop = InLoop;
    params.Bus = m_Bus;
    params.Spatial = m_Spatial;
    params.Position = GetPosition();
    params.Velocity = m_Velocity;
    params.Attenuation = m_Attenuation;
    return params;
}
