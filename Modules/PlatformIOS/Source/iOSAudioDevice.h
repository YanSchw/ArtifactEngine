#pragma once
#include "Audio/AudioDevice.h"
#include "iOSAudioDevice.gen.h"

class iOSAudioDevice : public AudioDevice {
public:
    ARTIFACT_CLASS();

    virtual ~iOSAudioDevice();

    virtual bool Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) override;
    virtual void Close() override;

private:
    void* m_Unit = nullptr;
};
