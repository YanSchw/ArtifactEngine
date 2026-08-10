#include "Platform/PlatformHooks.h"

#include "Core/EngineConfig.h"

#include <chrono>
#include <thread>

PlatformHooks& PlatformHooks::Get() {
    static PlatformHooks* hooks = [] {
        for (const Class& candidate : Class::GetSubclassesOf(StaticClass())) {
            if (candidate == StaticClass())
                continue;
            if (Object* instance = Object::Create(candidate))
                return Cast<PlatformHooks>(instance);
        }
        return new PlatformHooks();
    }();
    return *hooks;
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

void PlatformHooks::RunMainLoop(const std::function<bool()>& InTick) {
    while (true) {
        const auto frameStart = std::chrono::steady_clock::now();
        if (!InTick()) {
            return;
        }
        WaitForFrameBudget(frameStart);
    }
}
