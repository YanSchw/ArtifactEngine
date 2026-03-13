#include "Engine.h"

void Engine::MainLoop() {
    m_PreviousTime = std::chrono::steady_clock::now();
    do {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = currentTime.time_since_epoch() - m_PreviousTime.time_since_epoch();

        m_DeltaTime = elapsed.count() / 1000000000.0;

        // Cap the Framerate in Application

        m_PreviousTime = currentTime;
    } while (MainTick(static_cast<float>(m_DeltaTime)));
}