#include "AlsaAudioDevice.h"
#include "Core/Log.h"

#include <algorithm>
#include <alsa/asoundlib.h>
#include <cerrno>
#include <cmath>

AlsaAudioDevice::~AlsaAudioDevice() {
    Close();
}

bool AlsaAudioDevice::Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) {
    Close();

    snd_pcm_t* pcm = nullptr;
    if (snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        return false;
    }

    const unsigned int latency = (unsigned int)((uint64_t)InDesc.BufferFrames * 2 * 1000000 / InDesc.SampleRate);
    m_Int16 = snd_pcm_set_params(pcm, SND_PCM_FORMAT_FLOAT_LE, SND_PCM_ACCESS_RW_INTERLEAVED, InDesc.Channels, InDesc.SampleRate, 1, latency) < 0;
    if (m_Int16 && snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, InDesc.Channels, InDesc.SampleRate, 1, latency) < 0) {
        snd_pcm_close(pcm);
        return false;
    }

    snd_pcm_uframes_t bufferFrames = 0;
    snd_pcm_uframes_t periodFrames = 0;
    snd_pcm_get_params(pcm, &bufferFrames, &periodFrames);
    m_PeriodFrames = periodFrames > 0 ? (uint32_t)periodFrames : InDesc.BufferFrames;

    m_Format.SampleRate = InDesc.SampleRate;
    m_Format.Channels = InDesc.Channels;
    m_Format.SampleFormat = AudioSampleFormat::Float32;
    m_Render = InRender;
    m_Pcm = pcm;
    m_Running = true;
    m_Thread = std::thread([this] { RenderLoop(); });
    return true;
}

void AlsaAudioDevice::Close() {
    m_Running = false;
    if (m_Thread.joinable()) {
        m_Thread.join();
    }
    if (m_Pcm) {
        snd_pcm_drop((snd_pcm_t*)m_Pcm);
        snd_pcm_close((snd_pcm_t*)m_Pcm);
        m_Pcm = nullptr;
    }
}

void AlsaAudioDevice::RenderLoop() {
    snd_pcm_t* pcm = (snd_pcm_t*)m_Pcm;
    const uint32_t channels = m_Format.Channels;
    Array<float> samples((size_t)m_PeriodFrames * channels);
    Array<int16_t> converted(m_Int16 ? samples.Size() : 0);

    while (m_Running) {
        Render(samples.Data(), m_PeriodFrames);
        for (int32_t i = 0; i < converted.Size(); i++) {
            converted[i] = (int16_t)std::lround(std::clamp(samples[i], -1.0f, 1.0f) * 32767.0f);
        }

        const byte* data = m_Int16 ? (const byte*)converted.Data() : (const byte*)samples.Data();
        const size_t frameBytes = (m_Int16 ? sizeof(int16_t) : sizeof(float)) * channels;
        uint32_t written = 0;
        while (written < m_PeriodFrames && m_Running) {
            snd_pcm_sframes_t result = snd_pcm_writei(pcm, data + written * frameBytes, m_PeriodFrames - written);
            if (result == -EAGAIN) {
                continue;
            }
            if (result < 0) {
                result = snd_pcm_recover(pcm, (int)result, 1);
                if (result < 0) {
                    AE_ERROR("ALSA playback failed: {0}", snd_strerror((int)result));
                    m_Running = false;
                }
                continue;
            }
            written += (uint32_t)result;
        }
    }
}
