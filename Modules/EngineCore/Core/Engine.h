#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Engine.gen.h"

#include <chrono>

class GameInstance;

class Engine : public Object {
public:
    ARTIFACT_CLASS();

    Engine();
    static Engine& Get();

    GameInstance* GetGameInstance() const;
    class RenderPipeline* GetRenderPipeline() const { return m_RenderPipeline.Get(); }

    virtual void RequestExit(bool InForce);

protected:
    virtual void Initialize() = 0;
    virtual bool MainTick(double InDeltaTime) = 0;
    virtual void Shutdown() = 0;

    // Called by MainLoop every frame before MainTick, so input is fresh for
    // gameplay. EngineCore can't depend on InputSystem (that would be a circular
    // module dependency), so engine modules override this to tick it.
    virtual void TickInput(double InDeltaTime) {}

    void MainLoop();

protected:
    std::chrono::steady_clock::time_point m_PreviousTime;
    double m_DeltaTime;
    SharedObjectPtr<class RenderPipeline> m_RenderPipeline;
    SharedObjectPtr<class GameInstance> m_GameInstance;

    inline static bool s_IsRunning = true;

    friend int ArtifactMain(const Array<String>&);
};