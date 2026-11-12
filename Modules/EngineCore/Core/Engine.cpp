#include "Engine.h"

#include "EngineConfig.h"
#include "EngineSubsystem.h"
#include "Audio/AudioEngine.h"
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
    for (const SharedObjectPtr<EngineSubsystem>& subsystem : m_Subsystems) {
        subsystem->Tick(m_DeltaTime);
    }
    const bool keepRunning = MainTick(m_DeltaTime);
    if (AudioEngine::IsAvailable()) {
        AudioEngine::Get().Update();
    }
    return keepRunning && s_IsRunning;
}

void Engine::MainLoop() {
    m_PreviousTime = std::chrono::steady_clock::now();
    PlatformHooks::Get().RunMainLoop([this] { return Tick(); });
}

EngineSubsystem* Engine::GetSubsystem(const Class& InClass) const {
    for (const SharedObjectPtr<EngineSubsystem>& subsystem : m_Subsystems) {
        if (subsystem->IsA(InClass)) {
            return subsystem.Get();
        }
    }
    return nullptr;
}

void Engine::InitializeSubsystems() {
    for (const Class& subsystemClass : Class::GetSubclassesOf(EngineSubsystem::StaticClass())) {
        if (subsystemClass == EngineSubsystem::StaticClass()) {
            continue;
        }
        if (EngineSubsystem* subsystem = Cast<EngineSubsystem>(Object::Create(subsystemClass))) {
            m_Subsystems.Add(SharedObjectPtr<EngineSubsystem>(subsystem));
            subsystem->Initialize();
        }
    }
}

void Engine::ShutdownSubsystems() {
    for (int32_t i = m_Subsystems.Last(); i >= 0; i--) {
        m_Subsystems[i]->Shutdown();
    }
    m_Subsystems.Clear();
}

Engine& Engine::Get() {
    return *s_Instance;
}
