#pragma once
#include <cstdint>
#include <functional>
#include <memory>

class Win64WasapiStream {
public:
    using RenderFunction = std::function<void(float* OutSamples, uint32_t InFrameCount, uint32_t InSampleRate)>;
    struct State;

    Win64WasapiStream();
    ~Win64WasapiStream();

    bool Start(uint32_t InChannels, uint32_t InBufferFrames, const RenderFunction& InRender);
    void Stop();
    uint32_t GetSampleRate() const;

private:
    std::unique_ptr<State> m_State;
};
