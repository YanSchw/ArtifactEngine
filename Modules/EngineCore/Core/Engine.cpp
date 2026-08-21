#include "Engine.h"

#include "EngineConfig.h"
#include "GameFramework/DebugDraw.h"
#include "Platform/PlatformHooks.h"

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

bool Engine::Tick() {
    const auto frameStart = std::chrono::steady_clock::now();
    m_DeltaTime = std::chrono::duration<double>(frameStart - m_PreviousTime).count();
    m_PreviousTime = frameStart;

    DebugDraw::AdvanceFrame((float)m_DeltaTime);

    // Refresh input before gameplay reads it this frame.
    TickInput(m_DeltaTime);
    return MainTick(m_DeltaTime) && s_IsRunning;
}

void Engine::MainLoop() {
    m_PreviousTime = std::chrono::steady_clock::now();
    PlatformHooks::Get().RunMainLoop([this] { return Tick(); });
}

Engine& Engine::Get() {
    return *s_Instance;
}
