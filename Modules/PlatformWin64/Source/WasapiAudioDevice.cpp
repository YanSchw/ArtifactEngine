#include "WasapiAudioDevice.h"
#include "Win64Wasapi.h"

WasapiAudioDevice::WasapiAudioDevice() = default;

WasapiAudioDevice::~WasapiAudioDevice() {
    Close();
}

bool WasapiAudioDevice::Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) {
    Close();

    m_Render = InRender;
    m_Stream = std::make_unique<Win64WasapiStream>();
    const uint32_t channels = InDesc.Channels;
    const bool started = m_Stream->Start(channels, InDesc.BufferFrames, [this, channels](float* OutSamples, uint32_t InFrameCount, uint32_t InSampleRate) {
        m_Render(AudioFormat{ InSampleRate, channels, AudioSampleFormat::Float32 }, OutSamples, InFrameCount);
    });
    if (!started) {
        m_Stream = nullptr;
        return false;
    }

    m_Format.SampleRate = m_Stream->GetSampleRate();
    m_Format.Channels = channels;
    m_Format.SampleFormat = AudioSampleFormat::Float32;
    return true;
}

void WasapiAudioDevice::Close() {
    m_Stream = nullptr;
}
