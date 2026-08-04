#pragma once
#include "Platform/PlatformHooks.h"
#include "MacOSPlatformHooks.gen.h"

class MacOSPlatformHooks : public PlatformHooks {
public:
    ARTIFACT_CLASS();

    virtual void SetApplicationIcon(const String& InImagePath) override;
    virtual void SetSystemShortcutsSuppressed(bool InSuppressed) override;
    virtual bool SetWindowFullscreen(void* InNativeWindow, bool InFullscreen) override;
};
