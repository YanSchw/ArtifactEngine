#include "AudioEngine.h"
#include "AudioDevice.h"
#include "Assets/AssetManager.h"
#include "Assets/AudioClip.h"
#include "Core/EngineConfig.h"

#include <algorithm>
#include <cmath>
#include <numbers>

static AudioEngine* s_Instance = nullptr;
static constexpr float s_DeclickTime = 0.005f;

AudioEngine& AudioEngine::Get() {
    AE_ASSERT(s_Instance, "AudioEngine instance is not initialized yet!");
    return *s_Instance;
}

bool AudioEngine::IsAvailable() {
    return s_Instance != nullptr;
}

void AudioEngine::Initialize(bool InOpenDevice) {
    AE_ASSERT(!s_Instance, "AudioEngine instance is already initialized!");
    s_Instance = this;

    m_Buses.Add(AudioBus{ "Master" });
    m_MaxVoices = std::max(EngineConfig::GetConfigVar<int>("AudioMaxVoices"), 1);
    if (!InOpenDevice) {
        return;
    }

    AudioDeviceDesc desc;
    desc.SampleRate = (uint32_t)std::max(EngineConfig::GetConfigVar<int>("AudioSampleRate"), 8000);
    desc.BufferFrames = (uint32_t)std::max(EngineConfig::GetConfigVar<int>("AudioBufferFrames"), 64);

    m_Device = AudioDevice::Create();
    if (!m_Device) {
        return;
    }
    const bool opened = m_Device->Open(desc, [this](const AudioFormat& InFormat, float* OutSamples, uint32_t InFrameCount) {
        Render(InFormat, OutSamples, InFrameCount);
    });
    if (!opened) {
        AE_WARN("The audio device '{0}' failed to open, audio stays silent", m_Device->GetClass().Name);
        m_Device = nullptr;
        return;
    }
    AE_INFO("Audio device '{0}' opened: {1} Hz, {2} channels", m_Device->GetClass().Name, m_Device->GetFormat().SampleRate, m_Device->GetFormat().Channels);
}

void AudioEngine::Shutdown() {
    AE_ASSERT(s_Instance, "AudioEngine instance is not initialized!");
    delete s_Instance;
    s_Instance = nullptr;
}

AudioEngine::~AudioEngine() {
    if (m_Device) {
        m_Device->Close();
        m_Device = nullptr;
    }
    s_Instance = nullptr;
}

void AudioEngine::Update() {
    if (m_Device) {
        m_Device->Pump();
    }
    std::lock_guard lock(m_Mutex);
    RemoveFinishedVoices();
}

AudioHandle AudioEngine::Play(AudioClip* InClip, const AudioPlayParams& InParams) {
    if (!InClip) {
        return {};
    }
    if (!InClip->IsLoaded()) {
        AssetManager::Get().LoadAsset(InClip);
    }
    return Play(InClip->GetSamples(), InParams);
}

AudioHandle AudioEngine::Play(const SharedAudioSamples& InSamples, const AudioPlayParams& InParams) {
    if (!InSamples || InSamples->IsEmpty() || InSamples->GetSampleRate() == 0) {
        return {};
    }

    std::lock_guard lock(m_Mutex);
    RemoveFinishedVoices();
    if (m_Voices.Size() >= m_MaxVoices) {
        StealVoice();
    }

    AudioVoice voice;
    voice.Id = m_NextVoiceId++;
    voice.Samples = InSamples;
    voice.OwningWorld = InParams.OwningWorld;
    voice.Bus = FindOrAddBus(InParams.Bus);
    voice.Volume = std::max(InParams.Volume, 0.0f);
    voice.Pitch = std::max(InParams.Pitch, 0.01f);
    voice.Pan = std::clamp(InParams.Pan, -1.0f, 1.0f);
    voice.Loop = InParams.Loop;
    voice.Paused = InParams.StartPaused;
    voice.Spatial = InParams.Spatial;
    voice.Position = InParams.Position;
    voice.Velocity = InParams.Velocity;
    voice.Attenuation = InParams.Attenuation;

    const double frameCount = (double)InSamples->GetFrameCount();
    const double start = std::max(InParams.StartTime, 0.0) * InSamples->GetSampleRate();
    voice.Cursor = InParams.Loop ? std::fmod(start, frameCount) : std::min(start, frameCount - 1.0);

    if (InParams.FadeInTime > 0.0f) {
        voice.Fade = 0.0f;
        voice.FadeRate = 1.0f / InParams.FadeInTime;
    }

    m_Voices.Add(voice);
    return AudioHandle{ voice.Id };
}

void AudioEngine::StopAll(float InFadeOutTime) {
    std::lock_guard lock(m_Mutex);
    for (AudioVoice& voice : m_Voices) {
        StopVoice(voice, InFadeOutTime);
    }
}

void AudioEngine::StopAll(World* InOwningWorld, float InFadeOutTime) {
    std::lock_guard lock(m_Mutex);
    for (AudioVoice& voice : m_Voices) {
        if (voice.OwningWorld == InOwningWorld) {
            StopVoice(voice, InFadeOutTime);
        }
    }
}

AudioListener AudioEngine::GetListener() const {
    std::lock_guard lock(m_Mutex);
    return m_Listener;
}

void AudioEngine::SetListener(const AudioListener& InListener) {
    std::lock_guard lock(m_Mutex);
    m_Listener = InListener;
}

void AudioEngine::CreateBus(const String& InName, const String& InParent) {
    std::lock_guard lock(m_Mutex);
    const int32_t parent = FindOrAddBus(InParent);
    const int32_t bus = FindOrAddBus(InName);
    if (bus == 0) {
        return;
    }
    for (int32_t ancestor = parent; ancestor >= 0; ancestor = m_Buses[ancestor].Parent) {
        if (ancestor == bus) {
            AE_ERROR("Audio bus '{0}' cannot be parented to its own descendant '{1}'", InName, InParent);
            return;
        }
    }
    m_Buses[bus].Parent = parent;
}

float AudioEngine::GetBusVolume(const String& InBus) const {
    std::lock_guard lock(m_Mutex);
    const int32_t bus = FindBus(InBus);
    return bus >= 0 ? m_Buses[bus].Volume : 1.0f;
}

void AudioEngine::SetBusVolume(const String& InBus, float InVolume) {
    std::lock_guard lock(m_Mutex);
    m_Buses[FindOrAddBus(InBus)].Volume = std::max(InVolume, 0.0f);
}

bool AudioEngine::IsBusMuted(const String& InBus) const {
    std::lock_guard lock(m_Mutex);
    const int32_t bus = FindBus(InBus);
    return bus >= 0 && m_Buses[bus].Muted;
}

void AudioEngine::SetBusMuted(const String& InBus, bool InMuted) {
    std::lock_guard lock(m_Mutex);
    m_Buses[FindOrAddBus(InBus)].Muted = InMuted;
}

bool AudioEngine::IsBusPaused(const String& InBus) const {
    std::lock_guard lock(m_Mutex);
    const int32_t bus = FindBus(InBus);
    return bus >= 0 && m_Buses[bus].Paused;
}

void AudioEngine::SetBusPaused(const String& InBus, bool InPaused) {
    std::lock_guard lock(m_Mutex);
    m_Buses[FindOrAddBus(InBus)].Paused = InPaused;
}

int32_t AudioEngine::GetPlayingVoiceCount() const {
    std::lock_guard lock(m_Mutex);
    int32_t count = 0;
    for (const AudioVoice& voice : m_Voices) {
        if (!voice.Finished && !voice.Paused) {
            count++;
        }
    }
    return count;
}

AudioVoice* AudioEngine::FindVoice(uint64_t InId) {
    auto found = std::lower_bound(m_Voices.begin(), m_Voices.end(), InId, [](const AudioVoice& InVoice, uint64_t InValue) {
        return InVoice.Id < InValue;
    });
    return found != m_Voices.end() && found->Id == InId ? &*found : nullptr;
}

bool AudioEngine::VisitVoice(uint64_t InId, const std::function<void(AudioVoice&)>& InVisitor) {
    std::lock_guard lock(m_Mutex);
    AudioVoice* voice = FindVoice(InId);
    if (!voice || voice->Finished) {
        return false;
    }
    InVisitor(*voice);
    return true;
}

void AudioEngine::StopVoice(AudioVoice& OutVoice, float InFadeOutTime) {
    if (OutVoice.Finished) {
        return;
    }
    if (OutVoice.Paused || !OutVoice.Started || OutVoice.Fade <= 0.0f) {
        OutVoice.Finished = true;
        return;
    }
    const float fadeRate = -OutVoice.Fade / std::max(InFadeOutTime, s_DeclickTime);
    OutVoice.FadeRate = OutVoice.Stopping ? std::min(OutVoice.FadeRate, fadeRate) : fadeRate;
    OutVoice.Stopping = true;
}

void AudioEngine::StealVoice() {
    int32_t victim = -1;
    for (int32_t i = 0; i < m_Voices.Size() && victim < 0; i++) {
        if (!m_Voices[i].Loop) {
            victim = i;
        }
    }
    m_Voices.RemoveAt(victim >= 0 ? victim : 0);
}

void AudioEngine::RemoveFinishedVoices() {
    m_Voices.Filter([](const AudioVoice& InVoice) { return !InVoice.Finished; });
}

int32_t AudioEngine::FindBus(const String& InName) const {
    for (int32_t i = 0; i < m_Buses.Size(); i++) {
        if (m_Buses[i].Name == InName) {
            return i;
        }
    }
    return -1;
}

int32_t AudioEngine::FindOrAddBus(const String& InName) {
    if (InName.empty()) {
        return 0;
    }
    const int32_t existing = FindBus(InName);
    if (existing >= 0) {
        return existing;
    }
    m_Buses.Add(AudioBus{ InName, 0 });
    return m_Buses.Last();
}

float AudioEngine::GetBusGain(int32_t InBus) const {
    float gain = 1.0f;
    for (int32_t bus = InBus; bus >= 0; bus = m_Buses[bus].Parent) {
        gain *= m_Buses[bus].Muted ? 0.0f : m_Buses[bus].Volume;
    }
    return gain;
}

bool AudioEngine::IsBusPausedInHierarchy(int32_t InBus) const {
    for (int32_t bus = InBus; bus >= 0; bus = m_Buses[bus].Parent) {
        if (m_Buses[bus].Paused) {
            return true;
        }
    }
    return false;
}

float AudioEngine::ComputeDoppler(const AudioVoice& InVoice) const {
    const Vec3 toListener = m_Listener.Position - InVoice.Position;
    const float distance = glm::length(toListener);
    if (m_DopplerFactor <= 0.0f || m_SpeedOfSound <= 0.0f || distance < 1e-4f) {
        return 1.0f;
    }

    const Vec3 direction = toListener / distance;
    const float limit = 0.5f * m_SpeedOfSound / m_DopplerFactor;
    const float listenerSpeed = std::clamp(glm::dot(direction, m_Listener.Velocity), -limit, limit);
    const float sourceSpeed = std::clamp(glm::dot(direction, InVoice.Velocity), -limit, limit);
    const float shift = (m_SpeedOfSound - m_DopplerFactor * listenerSpeed) / (m_SpeedOfSound - m_DopplerFactor * sourceSpeed);
    return std::clamp(shift, 0.5f, 2.0f);
}

void AudioEngine::ComputeGains(const AudioVoice& InVoice, uint32_t InSourceChannels, uint32_t InOutputChannels, float OutGains[2][2]) const {
    float gain = InVoice.Volume * GetBusGain(InVoice.Bus);
    float pan = InVoice.Pan;
    if (InVoice.Spatial) {
        const Vec3 toSource = InVoice.Position - m_Listener.Position;
        const float distance = glm::length(toSource);
        gain *= InVoice.Attenuation.Evaluate(distance);
        pan = distance > 1e-4f ? std::clamp((glm::inverse(m_Listener.Rotation) * (toSource / distance)).x, -1.0f, 1.0f) : 0.0f;
    }

    OutGains[0][0] = OutGains[0][1] = OutGains[1][0] = OutGains[1][1] = 0.0f;
    const float share = 1.0f / InSourceChannels;

    if (InOutputChannels == 1) {
        for (uint32_t source = 0; source < InSourceChannels; source++) {
            OutGains[source][0] = gain * share;
        }
        return;
    }

    if (InVoice.Spatial) {
        const float angle = (pan + 1.0f) * std::numbers::pi_v<float> * 0.25f;
        for (uint32_t source = 0; source < InSourceChannels; source++) {
            OutGains[source][0] = gain * share * std::cos(angle);
            OutGains[source][1] = gain * share * std::sin(angle);
        }
        return;
    }

    const float left = gain * std::min(1.0f, 1.0f - pan);
    const float right = gain * std::min(1.0f, 1.0f + pan);
    if (InSourceChannels == 1) {
        OutGains[0][0] = left;
        OutGains[0][1] = right;
    } else {
        OutGains[0][0] = left;
        OutGains[1][1] = right;
    }
}

void AudioEngine::MixVoice(AudioVoice& OutVoice, const AudioFormat& InFormat, float* OutSamples, uint32_t InFrameCount) {
    const AudioSampleBuffer& samples = *OutVoice.Samples;
    const uint64_t frameCount = samples.GetFrameCount();
    const uint32_t sourceChannels = std::min(samples.GetChannels(), 2u);
    const uint32_t outputChannels = std::min(InFormat.Channels, 2u);

    float targetGains[2][2];
    ComputeGains(OutVoice, sourceChannels, outputChannels, targetGains);
    if (!OutVoice.Started) {
        std::copy(&targetGains[0][0], &targetGains[0][0] + 4, &OutVoice.Gains[0][0]);
        OutVoice.Started = true;
    }

    const float doppler = OutVoice.Spatial ? ComputeDoppler(OutVoice) : 1.0f;
    const double step = (double)samples.GetSampleRate() / InFormat.SampleRate * OutVoice.Pitch * doppler;
    const float rampStep = 1.0f / InFrameCount;

    for (uint32_t frame = 0; frame < InFrameCount; frame++) {
        const uint64_t index = std::min((uint64_t)OutVoice.Cursor, frameCount - 1);
        const uint64_t next = index + 1 < frameCount ? index + 1 : (OutVoice.Loop ? 0 : index);
        const float t = (float)(OutVoice.Cursor - (double)index);

        float source[2];
        for (uint32_t channel = 0; channel < sourceChannels; channel++) {
            const float a = samples.GetSample(index, channel);
            source[channel] = a + (samples.GetSample(next, channel) - a) * t;
        }

        const float ramp = (frame + 1) * rampStep;
        float* out = OutSamples + (size_t)frame * InFormat.Channels;
        for (uint32_t output = 0; output < outputChannels; output++) {
            float sum = 0.0f;
            for (uint32_t channel = 0; channel < sourceChannels; channel++) {
                const float from = OutVoice.Gains[channel][output];
                sum += source[channel] * (from + (targetGains[channel][output] - from) * ramp);
            }
            out[output] += sum * OutVoice.Fade;
        }

        if (OutVoice.FadeRate != 0.0f) {
            OutVoice.Fade = std::clamp(OutVoice.Fade + OutVoice.FadeRate / InFormat.SampleRate, 0.0f, 1.0f);
            if (OutVoice.Fade <= 0.0f && OutVoice.Stopping) {
                OutVoice.Finished = true;
                break;
            }
            if (OutVoice.Fade >= 1.0f && OutVoice.FadeRate > 0.0f) {
                OutVoice.FadeRate = 0.0f;
            }
        }

        OutVoice.Cursor += step;
        if (OutVoice.Cursor >= (double)frameCount) {
            if (!OutVoice.Loop) {
                OutVoice.Finished = true;
                break;
            }
            OutVoice.Cursor = std::fmod(OutVoice.Cursor, (double)frameCount);
        }
    }

    std::copy(&targetGains[0][0], &targetGains[0][0] + 4, &OutVoice.Gains[0][0]);
}

void AudioEngine::Render(const AudioFormat& InFormat, float* OutSamples, uint32_t InFrameCount) {
    const size_t sampleCount = (size_t)InFrameCount * InFormat.Channels;
    std::fill(OutSamples, OutSamples + sampleCount, 0.0f);
    if (InFrameCount == 0 || InFormat.SampleRate == 0) {
        return;
    }

    std::lock_guard lock(m_Mutex);
    for (AudioVoice& voice : m_Voices) {
        if (!voice.Finished && !voice.Paused && !IsBusPausedInHierarchy(voice.Bus)) {
            MixVoice(voice, InFormat, OutSamples, InFrameCount);
        }
    }
    for (size_t i = 0; i < sampleCount; i++) {
        OutSamples[i] = std::clamp(OutSamples[i], -1.0f, 1.0f);
    }
}

bool AudioHandle::Visit(const std::function<void(AudioVoice&)>& InVisitor) const {
    return IsValid() && AudioEngine::IsAvailable() && AudioEngine::Get().VisitVoice(Id, InVisitor);
}

bool AudioHandle::IsPlaying() const {
    bool playing = false;
    Visit([&](AudioVoice& InVoice) { playing = !InVoice.Paused && !InVoice.Stopping; });
    return playing;
}

bool AudioHandle::IsPaused() const {
    bool paused = false;
    Visit([&](AudioVoice& InVoice) { paused = InVoice.Paused && !InVoice.Stopping; });
    return paused;
}

void AudioHandle::Stop(float InFadeOutTime) const {
    Visit([&](AudioVoice& OutVoice) { AudioEngine::Get().StopVoice(OutVoice, InFadeOutTime); });
}

void AudioHandle::SetPaused(bool InPaused) const {
    Visit([&](AudioVoice& OutVoice) { OutVoice.Paused = InPaused; });
}

void AudioHandle::SetVolume(float InVolume) const {
    Visit([&](AudioVoice& OutVoice) { OutVoice.Volume = std::max(InVolume, 0.0f); });
}

void AudioHandle::SetPitch(float InPitch) const {
    Visit([&](AudioVoice& OutVoice) { OutVoice.Pitch = std::max(InPitch, 0.01f); });
}

void AudioHandle::SetPan(float InPan) const {
    Visit([&](AudioVoice& OutVoice) { OutVoice.Pan = std::clamp(InPan, -1.0f, 1.0f); });
}

void AudioHandle::SetLooping(bool InLoop) const {
    Visit([&](AudioVoice& OutVoice) { OutVoice.Loop = InLoop; });
}

void AudioHandle::SetPosition(const Vec3& InPosition, const Vec3& InVelocity) const {
    Visit([&](AudioVoice& OutVoice) {
        OutVoice.Position = InPosition;
        OutVoice.Velocity = InVelocity;
    });
}

bool AudioHandle::ApplyParams(const AudioPlayParams& InParams) const {
    return Visit([&](AudioVoice& OutVoice) {
        OutVoice.Volume = std::max(InParams.Volume, 0.0f);
        OutVoice.Pitch = std::max(InParams.Pitch, 0.01f);
        OutVoice.Pan = std::clamp(InParams.Pan, -1.0f, 1.0f);
        OutVoice.Loop = InParams.Loop;
        OutVoice.Position = InParams.Position;
        OutVoice.Velocity = InParams.Velocity;
        OutVoice.Attenuation = InParams.Attenuation;
    });
}

double AudioHandle::GetTime() const {
    double time = 0.0;
    Visit([&](AudioVoice& InVoice) { time = InVoice.Cursor / InVoice.Samples->GetSampleRate(); });
    return time;
}

void AudioHandle::SetTime(double InSeconds) const {
    Visit([&](AudioVoice& OutVoice) {
        const double frameCount = (double)OutVoice.Samples->GetFrameCount();
        OutVoice.Cursor = std::clamp(InSeconds * OutVoice.Samples->GetSampleRate(), 0.0, frameCount - 1.0);
    });
}
