#pragma once
#include "Assert.h"

/** The thread the engine runs its frame on. */
class MainThread {
public:
    static void Register();

    /** False on worker threads, and on the main thread while it runs a LocalUpdate chunk */
    static bool IsCurrent();

    struct LocalUpdateScope {
        LocalUpdateScope();
        ~LocalUpdateScope();
    };
};

#define AE_ASSERT_MAIN_THREAD(InWhat) \
    AE_ASSERT(MainThread::IsCurrent(), InWhat " may only be used on the main thread; Node::LocalUpdate runs on worker threads.")
