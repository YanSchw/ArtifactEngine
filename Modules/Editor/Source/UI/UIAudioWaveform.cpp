#include "UIAudioWaveform.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>
#include <cmath>

UIAudioWaveform::UIAudioWaveform() {
    ClipChildren = true;
}

void UIAudioWaveform::RefreshPeaks() {
    if (m_PeakSource == Samples && m_PeaksSplit == SplitChannels) {
        return;
    }
    m_PeakSource = Samples;
    m_PeaksSplit = SplitChannels;
    m_Peaks.Clear();
    m_Lanes = 0;
    m_Buckets = 0;

    if (!Samples || Samples->IsEmpty() || Samples->GetChannels() == 0) {
        return;
    }

    const uint64_t frames = Samples->GetFrameCount();
    const uint32_t channels = Samples->GetChannels();
    m_Lanes = SplitChannels ? channels : 1;
    m_Buckets = (uint32_t)std::min<uint64_t>(frames, s_MaxBuckets);
    m_Peaks.Resize(m_Lanes * m_Buckets);

    for (uint32_t bucket = 0; bucket < m_Buckets; ++bucket) {
        const uint64_t begin = bucket * frames / m_Buckets;
        const uint64_t end = std::max(begin + 1, (bucket + 1) * frames / m_Buckets);
        for (uint32_t lane = 0; lane < m_Lanes; ++lane) {
            Vec2 peak(1.0f, -1.0f);
            const uint32_t firstChannel = SplitChannels ? lane : 0;
            const uint32_t lastChannel = SplitChannels ? lane : channels - 1;
            for (uint64_t frame = begin; frame < end; ++frame) {
                for (uint32_t channel = firstChannel; channel <= lastChannel; ++channel) {
                    const float sample = Samples->GetSample(frame, channel);
                    peak.x = std::min(peak.x, sample);
                    peak.y = std::max(peak.y, sample);
                }
            }
            m_Peaks[lane * m_Buckets + bucket] = peak;
        }
    }
}

void UIAudioWaveform::Paint(UIDrawList& OutDrawList) {
    RefreshPeaks();
    if (m_Lanes == 0 || m_Geometry.Size.x < 1.0f || m_Geometry.Size.y < 1.0f) {
        return;
    }

    const UIRectF content = GetContentRect();
    const uint32_t columns = (uint32_t)std::max(1.0f, std::floor(content.Size.x));
    const float gap = m_Lanes > 1 ? LaneGap : 0.0f;
    const float laneHeight = (content.Size.y - gap * (float)(m_Lanes - 1)) / (float)m_Lanes;
    const double duration = Samples->GetDuration();
    const float playedX = (PlayheadTime >= 0.0 && duration > 0.0)
        ? content.Position.x + (float)(PlayheadTime / duration) * content.Size.x
        : content.Position.x;

    for (uint32_t lane = 0; lane < m_Lanes; ++lane) {
        const float top = content.Position.y + (float)lane * (laneHeight + gap);
        const float center = top + laneHeight * 0.5f;
        const float halfHeight = laneHeight * 0.5f;

        if (CenterLineColor.a > 0.0f) {
            OutDrawList.AddRect(UIRectF(content.Position.x, center - 0.5f, content.Size.x, 1.0f), CenterLineColor, m_WorldMatrix);
        }

        const Vec2* peaks = m_Peaks.Data() + lane * m_Buckets;
        for (uint32_t column = 0; column < columns; ++column) {
            const uint32_t begin = (uint32_t)((uint64_t)column * m_Buckets / columns);
            const uint32_t end = std::max(begin + 1, (uint32_t)((uint64_t)(column + 1) * m_Buckets / columns));
            Vec2 peak(1.0f, -1.0f);
            for (uint32_t bucket = begin; bucket < end && bucket < m_Buckets; ++bucket) {
                peak.x = std::min(peak.x, peaks[bucket].x);
                peak.y = std::max(peak.y, peaks[bucket].y);
            }

            const float x = content.Position.x + (float)column;
            const float yTop = center - std::clamp(peak.y, -1.0f, 1.0f) * halfHeight;
            const float yBottom = center - std::clamp(peak.x, -1.0f, 1.0f) * halfHeight;
            const float height = std::max(1.0f, yBottom - yTop);
            const Vec4& color = (x < playedX) ? PlayedColor : WaveColor;
            OutDrawList.AddRect(UIRectF(x, std::min(yTop, center - 0.5f * height), 1.0f, height), color, m_WorldMatrix);
        }
    }

    if (PlayheadTime >= 0.0 && duration > 0.0 && PlayheadColor.a > 0.0f) {
        OutDrawList.AddRect(UIRectF(playedX - 0.75f, content.Position.y, 1.5f, content.Size.y), PlayheadColor, m_WorldMatrix);
    }
}

void UIAudioWaveform::Scrub(float InCursorX) {
    if (!Scrubbed || !Samples) {
        return;
    }
    const UIRectF content = GetContentRect();
    const float fraction = content.Size.x > 0.0f ? std::clamp((InCursorX - content.Position.x) / content.Size.x, 0.0f, 1.0f) : 0.0f;
    Scrubbed(fraction * Samples->GetDuration());
}

void UIAudioWaveform::OnPressed(const Vec2& InCursorPos) {
    Scrub(InCursorPos.x);
}

void UIAudioWaveform::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    Scrub(InCursorPos.x);
}
