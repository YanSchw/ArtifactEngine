#pragma once
#include "Core/Engine.h"
#include "AssetCooker.gen.h"

class AssetCookerEngine : public Engine {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Initialize() override;
    virtual bool MainTick(double InDeltaTime) override;
    virtual void Shutdown() override;
};