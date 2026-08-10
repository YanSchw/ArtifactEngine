#pragma once
#include "CoreMinimal.h"
#include "Common/Types.h"
#include <functional>
#include "Surface.gen.h"

struct SurfaceParams {
    String Title = "Artifact";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool Fullscreen = false;
};

/* A platform-agnostic surface class for rendering */
class Surface : public Object {
public:
    ARTIFACT_CLASS();

    Surface();
    virtual ~Surface();

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;

    virtual void Initialize(const SurfaceParams& InParams) { (void)InParams; }

    virtual bool IsMinimized() const { return false; }
    virtual bool ShouldClose() const { return false; }

    /** Drains the host's event queue into the engine. */
    virtual void ProcessEvents() { }

    /** Invoked when the host needs a frame outside the normal tick, e.g. during a live resize. */
    virtual void SetRedrawCallback(const std::function<void()>& InCallback) { (void)InCallback; }

    virtual void SetCursorLocked(bool InLocked) { (void)InLocked; }
    virtual bool IsCursorLocked() const { return false; }

    /** The surface the application presents into, built from Platform::GetMainSurfaceClass(). */
    static SharedObjectPtr<Surface> CreateMain(const SurfaceParams& InParams);
    static Surface* GetMain();
};
