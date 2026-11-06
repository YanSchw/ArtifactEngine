#pragma once
#include "GameFramework/UINode.h"
#include "Audio/AudioSampleBuffer.h"
#include <functional>
#include "UIAudioWaveform.gen.h"

class UIAudioWaveform : public UINode {
public:
    ARTIFACT_CLASS();

    UIAudioWaveform();

    SharedAudioSamples Samples;
    double PlayheadTime = -1.0;
    bool SplitChannels = true;
    float LaneGap = 6.0f;
    Vec4 WaveColor = Vec4(1.0f);
    Vec4 PlayedColor = Vec4(1.0f);
    Vec4 PlayheadColor = Vec4(1.0f);
    Vec4 CenterLineColor = Vec4(0.0f);
    std::function<void(double)> Scrubbed;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;

private:
    static constexpr uint32_t s_MaxBuckets = 4096;

    void RefreshPeaks();
    void Scrub(float InCursorX);

    SharedAudioSamples m_PeakSource;
    bool m_PeaksSplit = true;
    uint32_t m_Lanes = 0;
    uint32_t m_Buckets = 0;
    Array<Vec2> m_Peaks;
};
