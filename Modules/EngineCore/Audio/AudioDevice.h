#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Audio/AudioSampleBuffer.h"
#include <functional>
#include "AudioDevice.gen.h"

using AudioRenderCallback = std::function<void(const AudioFormat& InFormat, float* OutSamples, uint32_t InFrameCount)>;

struct AudioDeviceDesc {
    uint32_t SampleRate = 48000;
    uint32_t Channels = 2;
    uint32_t BufferFrames = 512;
};

class AudioDevice : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~AudioDevice() = default;

    virtual bool Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) = 0;
    virtual void Close() = 0;
    virtual void Pump() {}

    const AudioFormat& GetFormat() const { return m_Format; }
    void Render(float* OutSamples, uint32_t InFrameCount) const { m_Render(m_Format, OutSamples, InFrameCount); }

    static SharedObjectPtr<AudioDevice> Create();

protected:
    AudioFormat m_Format;
    AudioRenderCallback m_Render;
};
