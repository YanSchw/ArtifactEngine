#pragma once
#include "Object.h"
#include "Engine.gen.h"

#include <chrono>

class Engine : public Object {
public:
    ARTIFACT_CLASS();

    virtual void Initialize() = 0;
    virtual bool MainTick(double InDeltaTime) = 0;
    virtual void Shutdown() = 0;

    void MainLoop();

private:
    std::chrono::steady_clock::time_point m_PreviousTime;
    double m_DeltaTime;

};