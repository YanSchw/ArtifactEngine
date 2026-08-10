#pragma once
#include "Object/Object.h"
#include "Common/String.h"
#include <functional>
#include "PlatformHooks.gen.h"

/* Behaviour only some platforms have. Every hook defaults to doing nothing. */
class PlatformHooks : public Object {
public:
    ARTIFACT_CLASS();

    static PlatformHooks& Get();

    /** Drives the engine's frame loop until the tick returns false. The default runs it inline,
     *  paced by the CapFPS budget; hosts that own the main thread hand the tick to them instead. */
    virtual void RunMainLoop(const std::function<bool()>& InTick);

    /** Whether work can run off the main thread. */
    virtual bool SupportsBackgroundThreads() const { return true; }

    virtual void SetApplicationIcon(const String& InImagePath) { (void)InImagePath; }

    /** Suspends the OS-level keyboard shortcuts that swallow key combos. */
    virtual void SetSystemShortcutsSuppressed(bool InSuppressed) { (void)InSuppressed; }

    /** Puts a native window into the OS's own fullscreen presentation, returning false where the
     *  platform has none and the caller should fall back to a monitor-sized window. */
    virtual bool SetWindowFullscreen(void* InNativeWindow, bool InFullscreen) {
        (void)InNativeWindow;
        (void)InFullscreen;
        return false;
    }
};
