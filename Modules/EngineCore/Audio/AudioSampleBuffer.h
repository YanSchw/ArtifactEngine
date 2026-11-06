#pragma once
#include "Common/Array.h"
#include "Common/Types.h"
#include "Object/Enum.h"
#include <memory>
#include "AudioSampleBuffer.gen.h"

ARTIFACT_ENUM();
enum class AudioSampleFormat : uint8_t {
    Int16 = 0,
    Float32 = 1
};

struct AudioFormat {
    uint32_t SampleRate = 48000;
    uint32_t Channels = 2;
    AudioSampleFormat SampleFormat = AudioSampleFormat::Float32;

    uint32_t GetBytesPerSample() const { return SampleFormat == AudioSampleFormat::Int16 ? 2 : 4; }
    uint32_t GetBytesPerFrame() const { return GetBytesPerSample() * Channels; }
    uint32_t GetBitDepth() const { return GetBytesPerSample() * 8; }
    uint32_t GetBitrate() const { return SampleRate * Channels * GetBitDepth(); }

    bool operator==(const AudioFormat& InOther) const = default;
};

class AudioSampleBuffer {
public:
    AudioSampleBuffer() = default;
    AudioSampleBuffer(const AudioFormat& InFormat, uint64_t InFrameCount);

    static AudioSampleBuffer FromInterleaved(const float* InSamples, uint64_t InFrameCount, uint32_t InChannels, uint32_t InSampleRate);

    const AudioFormat& GetFormat() const { return m_Format; }
    uint32_t GetChannels() const { return m_Format.Channels; }
    uint32_t GetSampleRate() const { return m_Format.SampleRate; }
    uint64_t GetFrameCount() const { return m_FrameCount; }
    double GetDuration() const;
    bool IsEmpty() const { return m_FrameCount == 0; }

    Array<byte>& GetData() { return m_Data; }
    const Array<byte>& GetData() const { return m_Data; }

    float GetSample(uint64_t InFrame, uint32_t InChannel) const {
        const uint64_t index = InFrame * m_Format.Channels + InChannel;
        if (m_Format.SampleFormat == AudioSampleFormat::Int16) {
            return (float)((const int16_t*)m_Data.Data())[index] * (1.0f / 32768.0f);
        }
        return ((const float*)m_Data.Data())[index];
    }
    void SetSample(uint64_t InFrame, uint32_t InChannel, float InValue);
    void ApplyGain(float InGain);

    AudioSampleBuffer WithChannels(uint32_t InChannels) const;
    AudioSampleBuffer WithSampleRate(uint32_t InSampleRate) const;
    AudioSampleBuffer WithSampleFormat(AudioSampleFormat InSampleFormat) const;
    AudioSampleBuffer ConvertTo(const AudioFormat& InFormat) const;

private:
    AudioFormat m_Format;
    uint64_t m_FrameCount = 0;
    Array<byte> m_Data;
};

using SharedAudioSamples = std::shared_ptr<const AudioSampleBuffer>;
