#include "RenderTiming.h"
#include "RenderingAPI.h"

RenderTimer::RenderTimer() {
    RenderingAPI* api = RenderingAPI::GetInstance();
    m_Query = api ? api->AllocateTimestampQueryPair() : RenderingAPI::InvalidTimestampQuery;
}

RenderTimer::~RenderTimer() {
    if (RenderingAPI* api = RenderingAPI::GetInstance()) {
        api->ReleaseTimestampQueryPair(m_Query);
    }
}

void RenderTimer::Begin() {
    m_CpuBegin = std::chrono::steady_clock::now();
    if (m_Query != RenderingAPI::InvalidTimestampQuery) {
        RenderingAPI::GetInstance()->WriteTimestamp(m_Query);
    }
}

void RenderTimer::End() {
    if (m_Query != RenderingAPI::InvalidTimestampQuery) {
        RenderingAPI* api = RenderingAPI::GetInstance();
        api->WriteTimestamp(m_Query + 1);
        m_Timing.GpuMilliseconds = api->GetTimestampQueryMilliseconds(m_Query);
    }
    const std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - m_CpuBegin;
    m_Timing.CpuMilliseconds = elapsed.count();
}