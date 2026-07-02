#include "Engine.h"

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

void Engine::MainLoop() {
    m_PreviousTime = std::chrono::steady_clock::now();
    do {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = currentTime.time_since_epoch() - m_PreviousTime.time_since_epoch();

        m_DeltaTime = elapsed.count() / 1000000000.0;

        // Cap the Framerate in Application

        m_PreviousTime = currentTime;

        // Refresh input before gameplay reads it this frame.
        TickInput(m_DeltaTime);
    } while (MainTick(static_cast<float>(m_DeltaTime)) && s_IsRunning);
}

Engine& Engine::Get() {
    return *s_Instance;
}