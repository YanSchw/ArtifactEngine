#pragma once
#include "Platform/PlatformHooks.h"
#include "WebPlatformHooks.gen.h"

class WebPlatformHooks : public PlatformHooks {
public:
    ARTIFACT_CLASS();

    /** The browser owns the main thread, so the frame loop is handed to requestAnimationFrame
     *  instead of being run inline. */
    virtual void RunMainLoop(const std::function<bool()>& InTick) override;

    // Workers need a cross-origin isolated page; the package is meant to be served by any plain
    // HTTP server, so the engine stays single threaded here.
    virtual bool SupportsBackgroundThreads() const override { return false; }
};
