#include "AudioPreview.h"
#include "Assets/AudioClip.h"

void AudioPreview::Play(AudioClip* InClip, double InStartTime, bool InLoop, bool InPaused) {
    Stop();
    if (!InClip || !AudioEngine::IsAvailable()) {
        return;
    }

    AudioPlayParams params;
    params.Loop = InLoop;
    params.StartTime = InStartTime;
    params.StartPaused = InPaused;
    s_Handle = AudioEngine::Get().Play(InClip, params);
    s_Clip = s_Handle.IsValid() ? InClip : nullptr;
}

void AudioPreview::Toggle(AudioClip* InClip, bool InLoop) {
    if (IsPlaying(InClip)) {
        s_Handle.SetPaused(true);
    } else if (IsPaused(InClip)) {
        s_Handle.SetLooping(InLoop);
        s_Handle.SetPaused(false);
    } else {
        Play(InClip, 0.0, InLoop);
    }
}

void AudioPreview::Seek(AudioClip* InClip, double InTime, bool InLoop) {
    if (IsActive(InClip)) {
        s_Handle.SetTime(InTime);
    } else {
        Play(InClip, InTime, InLoop, true);
    }
}

void AudioPreview::Stop() {
    if (AudioEngine::IsAvailable()) {
        s_Handle.Stop();
    }
    s_Handle = {};
    s_Clip = nullptr;
}

void AudioPreview::Stop(const AudioClip* InClip) {
    if (Owns(InClip)) {
        Stop();
    }
}

void AudioPreview::SetLooping(const AudioClip* InClip, bool InLoop) {
    if (Owns(InClip)) {
        s_Handle.SetLooping(InLoop);
    }
}

bool AudioPreview::IsActive(const AudioClip* InClip) {
    return IsPlaying(InClip) || IsPaused(InClip);
}

bool AudioPreview::IsPlaying(const AudioClip* InClip) {
    return Owns(InClip) && s_Handle.IsPlaying();
}

bool AudioPreview::IsPaused(const AudioClip* InClip) {
    return Owns(InClip) && s_Handle.IsPaused();
}

double AudioPreview::GetTime(const AudioClip* InClip) {
    return Owns(InClip) ? s_Handle.GetTime() : 0.0;
}

bool AudioPreview::Owns(const AudioClip* InClip) {
    return InClip && s_Clip == InClip && AudioEngine::IsAvailable();
}
