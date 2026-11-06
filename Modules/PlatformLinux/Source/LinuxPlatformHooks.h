#pragma once
#include "Platform/PlatformHooks.h"
#include "LinuxPlatformHooks.gen.h"

class LinuxPlatformHooks : public PlatformHooks {
public:
    ARTIFACT_CLASS();

    virtual Array<String> OpenFilesDialog(const String& InTitle) override;
};
