#include "iOSPlatformHooks.h"

extern "C" {
    void* objc_autoreleasePoolPush(void);
    void objc_autoreleasePoolPop(void* InPool);
}

void iOSPlatformHooks::RunMainLoop(const std::function<bool()>& InTick) {
    PlatformHooks::RunMainLoop([&InTick] {
        void* pool = objc_autoreleasePoolPush();
        const bool keepRunning = InTick();
        objc_autoreleasePoolPop(pool);
        return keepRunning;
    });
}
