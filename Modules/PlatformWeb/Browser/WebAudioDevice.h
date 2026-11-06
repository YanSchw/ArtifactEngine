#pragma once
#include "Audio/AudioDevice.h"
#include "WebAudioDevice.gen.h"

class WebAudioDevice : public AudioDevice {
public:
    ARTIFACT_CLASS();

    virtual ~WebAudioDevice();

    virtual bool Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) override;
    virtual void Close() override;
    virtual void Pump() override;

private:
    Array<float> m_Samples;
    uint32_t m_BlockFrames = 512;
    uint32_t m_LeadFrames = 4800;
    bool m_Open = false;
};
