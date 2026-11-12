#pragma once
#include "Object/Object.h"
#include "EngineSubsystem.gen.h"

class EngineSubsystem : public Object {
public:
    ARTIFACT_CLASS();

    virtual void Initialize() {}
    virtual void Shutdown() {}
    virtual void Tick(double) {}
};
