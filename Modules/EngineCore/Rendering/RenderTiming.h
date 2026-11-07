#pragma once
#include "CoreMinimal.h"

#include <chrono>

struct RenderTiming {
    /** Time spent recording the commands. */
    double CpuMilliseconds = 0.0;
    /** Time the GPU spent executing them, a frame or two behind CpuMilliseconds. Stays 0 without timestamp support. */
    double GpuMilliseconds = 0.0;
};

class RenderTimer {
public:
    RenderTimer();
    ~RenderTimer();
    RenderTimer(const RenderTimer&) = delete;
    RenderTimer& operator=(const RenderTimer&) = delete;

    void Begin();
    void End();

    const RenderTiming& GetTiming() const { return m_Timing; }

private:
    RenderTiming m_Timing;
    std::chrono::steady_clock::time_point m_CpuBegin;
    uint32_t m_Query;
};