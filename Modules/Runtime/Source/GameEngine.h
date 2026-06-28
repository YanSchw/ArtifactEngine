#pragma once
#include "Core/Engine.h"
#include "GameEngine.gen.h"

class GameEngine : public Engine {
public:
    ARTIFACT_CLASS();
    
protected:
    virtual void Initialize() override;
    virtual bool MainTick(double InDeltaTime) override;
    virtual void TickInput(double InDeltaTime) override;
    virtual void Shutdown() override;
};