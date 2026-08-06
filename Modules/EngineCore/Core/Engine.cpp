#include "Engine.h"

#include "EngineConfig.h"

#include <thread>

static Engine* s_Instance = nullptr;

Engine::Engine() {
    s_Instance = this;
}

GameInstance* Engine::GetGameInstance() const {
    return m_GameInstance;
}

void Engine::RequestExit(bool InForce) {
    if (InForce) {
        AE_WARN("Forced engine exit requested. This will terminate the process immediately.");
        std::exit(0);
    } else {
        AE_INFO("Engine exit requested.");
        s_IsRunning = false;
    }
}

static void WaitForFrameBudget(std::chrono::steady_clock::time_point InFrameStart) {
    const int capFPS = EngineConfig::GetConfigVar<int>("CapFPS");
    if (capFPS <= 0) {
        return;
    }

    using namespace std::chrono;
    const auto frameEnd = InFrameStart + duration_cast<steady_clock::duration>(duration<double>(1.0 / capFPS));

    constexpr auto sleepMargin = milliseconds(2);
    const auto remaining = frameEnd - steady_clock::now();
    if (remaining > sleepMargin) {
        std::this_thread::sleep_for(remaining - sleepMargin);
    }
    while (steady_clock::now() < frameEnd) {
        std::this_thread::yield();
    }
}

void Engine::MainLoop() {
    m_PreviousTime = std::chrono::steady_clock::now();
    bool keepRunning = true;
    do {
        const auto frameStart = std::chrono::steady_clock::now();
        m_DeltaTime = std::chrono::duration<double>(frameStart - m_PreviousTime).count();
        m_PreviousTime = frameStart;

        // Refresh input before gameplay reads it this frame.
        TickInput(m_DeltaTime);
        keepRunning = MainTick(m_DeltaTime) && s_IsRunning;

        WaitForFrameBudget(frameStart);
    } while (keepRunning);
}

Engine& Engine::Get() {
    return *s_Instance;
}