#pragma once
#include "Platform/PlatformHooks.h"
#include "iOSPlatformHooks.gen.h"

class iOSPlatformHooks : public PlatformHooks {
public:
    ARTIFACT_CLASS();

    virtual void RunMainLoop(const std::function<bool()>& InTick) override;
};
