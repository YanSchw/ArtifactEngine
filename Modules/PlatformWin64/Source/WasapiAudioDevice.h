#pragma once
#include "Audio/AudioDevice.h"
#include <memory>
#include "WasapiAudioDevice.gen.h"

class Win64WasapiStream;

class WasapiAudioDevice : public AudioDevice {
public:
    ARTIFACT_CLASS();

    WasapiAudioDevice();
    virtual ~WasapiAudioDevice();

    virtual bool Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) override;
    virtual void Close() override;

private:
    std::unique_ptr<Win64WasapiStream> m_Stream;
};
