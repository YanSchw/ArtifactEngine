#pragma once
#include "Audio/AudioEngine.h"

class AudioClip;

class AudioPreview {
public:
    static void Play(AudioClip* InClip, double InStartTime = 0.0, bool InLoop = false, bool InPaused = false);
    static void Toggle(AudioClip* InClip, bool InLoop = false);
    static void Seek(AudioClip* InClip, double InTime, bool InLoop = false);
    static void Stop();
    static void Stop(const AudioClip* InClip);
    static void SetLooping(const AudioClip* InClip, bool InLoop);

    static bool IsActive(const AudioClip* InClip);
    static bool IsPlaying(const AudioClip* InClip);
    static bool IsPaused(const AudioClip* InClip);
    static double GetTime(const AudioClip* InClip);

private:
    static bool Owns(const AudioClip* InClip);

    inline static const AudioClip* s_Clip = nullptr;
    inline static AudioHandle s_Handle;
};
