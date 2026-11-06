#include "AudioSampleBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

AudioSampleBuffer::AudioSampleBuffer(const AudioFormat& InFormat, uint64_t InFrameCount)
    : m_Format(InFormat), m_FrameCount(InFrameCount) {
    m_Data.Resize((size_t)(InFrameCount * InFormat.GetBytesPerFrame()));
}

AudioSampleBuffer AudioSampleBuffer::FromInterleaved(const float* InSamples, uint64_t InFrameCount, uint32_t InChannels, uint32_t InSampleRate) {
    AudioSampleBuffer buffer({ InSampleRate, InChannels, AudioSampleFormat::Float32 }, InFrameCount);
    if (InFrameCount > 0) {
        memcpy(buffer.m_Data.Data(), InSamples, buffer.m_Data.Size());
    }
    return buffer;
}

double AudioSampleBuffer::GetDuration() const {
    return m_Format.SampleRate > 0 ? (double)m_FrameCount / m_Format.SampleRate : 0.0;
}

void AudioSampleBuffer::SetSample(uint64_t InFrame, uint32_t InChannel, float InValue) {
    const uint64_t index = InFrame * m_Format.Channels + InChannel;
    if (m_Format.SampleFormat == AudioSampleFormat::Int16) {
        ((int16_t*)m_Data.Data())[index] = (int16_t)std::lround(std::clamp(InValue, -1.0f, 1.0f) * 32767.0f);
    } else {
        ((float*)m_Data.Data())[index] = InValue;
    }
}

void AudioSampleBuffer::ApplyGain(float InGain) {
    if (InGain == 1.0f) {
        return;
    }
    for (uint64_t frame = 0; frame < m_FrameCount; frame++) {
        for (uint32_t channel = 0; channel < m_Format.Channels; channel++) {
            SetSample(frame, channel, GetSample(frame, channel) * InGain);
        }
    }
}

static Vec2 GetStereoImage(uint32_t InChannels, uint32_t InChannel) {
    constexpr float s_Side = std::numbers::sqrt2_v<float> * 0.5f;
    if (InChannels == 1) {
        return Vec2(1.0f, 1.0f);
    }
    if (InChannels == 2) {
        return InChannel == 0 ? Vec2(1.0f, 0.0f) : Vec2(0.0f, 1.0f);
    }

    enum Speaker { FL, FR, FC, LFE, BL, BR, BC, SL, SR };
    static const Array<Array<Speaker>> s_Layouts = {
        { FL, FR, FC },
        { FL, FR, BL, BR },
        { FL, FR, FC, BL, BR },
        { FL, FR, FC, LFE, BL, BR },
        { FL, FR, FC, LFE, BC, SL, SR },
        { FL, FR, FC, LFE, BL, BR, SL, SR },
    };
    if (InChannels > 8) {
        return InChannel % 2 == 0 ? Vec2(s_Side, 0.0f) : Vec2(0.0f, s_Side);
    }
    switch (s_Layouts[InChannels - 3][InChannel]) {
        case FL: return Vec2(1.0f, 0.0f);
        case FR: return Vec2(0.0f, 1.0f);
        case FC: return Vec2(s_Side, s_Side);
        case LFE: return Vec2(0.0f, 0.0f);
        case BL: case SL: return Vec2(s_Side, 0.0f);
        case BR: case SR: return Vec2(0.0f, s_Side);
        case BC: return Vec2(0.5f, 0.5f);
    }
    return Vec2(0.0f);
}

static float GetChannelGain(uint32_t InSourceChannels, uint32_t InSource, uint32_t InTargetChannels, uint32_t InTarget) {
    const Vec2 image = GetStereoImage(InSourceChannels, InSource);
    if (InTargetChannels == 1) {
        return 0.5f * (image.x + image.y);
    }
    if (InTarget > 1) {
        return 0.0f;
    }
    return InTarget == 0 ? image.x : image.y;
}

AudioSampleBuffer AudioSampleBuffer::WithChannels(uint32_t InChannels) const {
    if (InChannels == 0 || InChannels == m_Format.Channels) {
        return *this;
    }

    Array<float> gains((size_t)m_Format.Channels * InChannels);
    for (uint32_t target = 0; target < InChannels; target++) {
        for (uint32_t source = 0; source < m_Format.Channels; source++) {
            gains[target * m_Format.Channels + source] = GetChannelGain(m_Format.Channels, source, InChannels, target);
        }
    }

    AudioSampleBuffer result({ m_Format.SampleRate, InChannels, m_Format.SampleFormat }, m_FrameCount);
    for (uint64_t frame = 0; frame < m_FrameCount; frame++) {
        for (uint32_t target = 0; target < InChannels; target++) {
            float sum = 0.0f;
            for (uint32_t source = 0; source < m_Format.Channels; source++) {
                sum += GetSample(frame, source) * gains[target * m_Format.Channels + source];
            }
            result.SetSample(frame, target, sum);
        }
    }
    return result;
}

static constexpr int32_t s_KernelZeroCrossings = 16;
static constexpr int32_t s_KernelResolution = 1024;

static const Array<float>& GetResamplingKernel() {
    static const Array<float> s_Kernel = [] {
        Array<float> kernel((size_t)(s_KernelZeroCrossings * s_KernelResolution + 2));
        for (int32_t i = 0; i < kernel.Size(); i++) {
            const double x = (double)i / s_KernelResolution;
            const double sinc = x == 0.0 ? 1.0 : std::sin(std::numbers::pi * x) / (std::numbers::pi * x);
            const double u = std::min(x / s_KernelZeroCrossings, 1.0);
            const double blackman = 0.42 + 0.5 * std::cos(std::numbers::pi * u) + 0.08 * std::cos(2.0 * std::numbers::pi * u);
            kernel[i] = (float)(sinc * blackman);
        }
        return kernel;
    }();
    return s_Kernel;
}

static float SampleKernel(double InX) {
    const Array<float>& kernel = GetResamplingKernel();
    const double position = std::abs(InX) * s_KernelResolution;
    const int32_t index = (int32_t)position;
    if (index >= kernel.Size() - 1) {
        return 0.0f;
    }
    const float t = (float)(position - index);
    return kernel[index] + (kernel[index + 1] - kernel[index]) * t;
}

AudioSampleBuffer AudioSampleBuffer::WithSampleRate(uint32_t InSampleRate) const {
    if (InSampleRate == 0 || InSampleRate == m_Format.SampleRate || IsEmpty()) {
        AudioSampleBuffer result = *this;
        result.m_Format.SampleRate = InSampleRate == 0 ? m_Format.SampleRate : InSampleRate;
        return result;
    }

    const double ratio = (double)InSampleRate / m_Format.SampleRate;
    const double cutoff = std::min(1.0, ratio);
    const double halfWidth = s_KernelZeroCrossings / cutoff;
    const uint64_t frameCount = (uint64_t)std::llround(m_FrameCount * ratio);
    const int64_t lastSourceFrame = (int64_t)m_FrameCount - 1;

    AudioSampleBuffer result({ InSampleRate, m_Format.Channels, m_Format.SampleFormat }, frameCount);
    Array<float> weights((size_t)std::ceil(halfWidth * 2.0) + 2);
    Array<float> sums(m_Format.Channels);

    for (uint64_t frame = 0; frame < frameCount; frame++) {
        const double center = frame / ratio;
        const int64_t first = std::max<int64_t>((int64_t)std::ceil(center - halfWidth), 0);
        const int64_t last = std::min<int64_t>((int64_t)std::floor(center + halfWidth), lastSourceFrame);

        float weightSum = 0.0f;
        for (int64_t source = first; source <= last; source++) {
            const float weight = SampleKernel((center - source) * cutoff);
            weights[(int32_t)(source - first)] = weight;
            weightSum += weight;
        }
        const float normalize = std::abs(weightSum) > 1e-6f ? 1.0f / weightSum : 0.0f;

        std::fill(sums.begin(), sums.end(), 0.0f);
        for (int64_t source = first; source <= last; source++) {
            const float weight = weights[(int32_t)(source - first)];
            for (uint32_t channel = 0; channel < m_Format.Channels; channel++) {
                sums[channel] += GetSample((uint64_t)source, channel) * weight;
            }
        }
        for (uint32_t channel = 0; channel < m_Format.Channels; channel++) {
            result.SetSample(frame, channel, sums[channel] * normalize);
        }
    }
    return result;
}

AudioSampleBuffer AudioSampleBuffer::WithSampleFormat(AudioSampleFormat InSampleFormat) const {
    if (InSampleFormat == m_Format.SampleFormat) {
        return *this;
    }

    AudioSampleBuffer result({ m_Format.SampleRate, m_Format.Channels, InSampleFormat }, m_FrameCount);
    for (uint64_t frame = 0; frame < m_FrameCount; frame++) {
        for (uint32_t channel = 0; channel < m_Format.Channels; channel++) {
            result.SetSample(frame, channel, GetSample(frame, channel));
        }
    }
    return result;
}

AudioSampleBuffer AudioSampleBuffer::ConvertTo(const AudioFormat& InFormat) const {
    if (InFormat == m_Format) {
        return *this;
    }

    AudioSampleBuffer result = WithSampleFormat(AudioSampleFormat::Float32);
    if (InFormat.Channels < result.GetChannels()) {
        result = result.WithChannels(InFormat.Channels);
    }
    if (InFormat.SampleRate != result.GetSampleRate()) {
        result = result.WithSampleRate(InFormat.SampleRate);
    }
    if (InFormat.Channels > result.GetChannels()) {
        result = result.WithChannels(InFormat.Channels);
    }
    return result.WithSampleFormat(InFormat.SampleFormat);
}
