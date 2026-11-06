#pragma once
#include "Audio/AudioDevice.h"
#include "CoreAudioDevice.gen.h"

class CoreAudioDevice : public AudioDevice {
public:
    ARTIFACT_CLASS();

    virtual ~CoreAudioDevice();

    virtual bool Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) override;
    virtual void Close() override;

private:
    void* m_Unit = nullptr;
};
