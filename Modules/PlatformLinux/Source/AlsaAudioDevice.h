#pragma once
#include "Audio/AudioDevice.h"
#include <atomic>
#include <thread>
#include "AlsaAudioDevice.gen.h"

class AlsaAudioDevice : public AudioDevice {
public:
    ARTIFACT_CLASS();

    virtual ~AlsaAudioDevice();

    virtual bool Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) override;
    virtual void Close() override;

private:
    void RenderLoop();

    void* m_Pcm = nullptr;
    uint32_t m_PeriodFrames = 512;
    bool m_Int16 = false;
    std::atomic<bool> m_Running = false;
    std::thread m_Thread;
};
