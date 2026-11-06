#pragma once
#include "Platform/PlatformHooks.h"
#include "Win64PlatformHooks.gen.h"

class Win64PlatformHooks : public PlatformHooks {
public:
    ARTIFACT_CLASS();

    virtual Array<String> OpenFilesDialog(const String& InTitle) override;
};
