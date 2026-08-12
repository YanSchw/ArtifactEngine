#include "JoltSystem.h"
#include "Core/Log.h"
#include "Platform/PlatformHooks.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>
#include <cstdarg>
#include <cstdio>
#include <thread>

namespace JoltSystem {

    static int32_t s_RefCount = 0;
    static JPH::TempAllocator* s_TempAllocator = nullptr;
    static JPH::JobSystem* s_JobSystem = nullptr;

    static constexpr uint32_t TempAllocatorSize = 16 * 1024 * 1024;

    static void TraceImpl(const char* InFormat, ...) {
        char message[1024];
        va_list args;
        va_start(args, InFormat);
        vsnprintf(message, sizeof(message), InFormat, args);
        va_end(args);
        AE_INFO("[Jolt] {0}", message);
    }

    static JPH::JobSystem* CreateJobSystem() {
        if (!PlatformHooks::Get().SupportsBackgroundThreads()) {
            return new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);
        }

        const uint32_t cores = std::thread::hardware_concurrency();
        const int32_t workers = (int32_t)(cores > 1 ? cores - 1 : 1);
        return new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, workers);
    }

    void Acquire() {
        if (s_RefCount++ > 0) {
            return;
        }

        JPH::RegisterDefaultAllocator();
        JPH::Trace = TraceImpl;
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        s_TempAllocator = new JPH::TempAllocatorImpl(TempAllocatorSize);
        s_JobSystem = CreateJobSystem();
    }

    void Release() {
        if (--s_RefCount > 0) {
            return;
        }

        delete s_JobSystem;
        s_JobSystem = nullptr;
        delete s_TempAllocator;
        s_TempAllocator = nullptr;

        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    JPH::TempAllocator& GetTempAllocator() {
        return *s_TempAllocator;
    }

    JPH::JobSystem& GetJobSystem() {
        return *s_JobSystem;
    }

}
