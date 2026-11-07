#pragma once
#include "CoreMinimal.h"
#include "RenderingAPI.h"
#include "RenderTiming.h"
#include "RenderPass.gen.h"

class RenderStage;

/** What every stage and pass of one RenderPipeline::Render call works from. */
struct RenderContext {
    double DeltaTime = 0.0;
    uint32_t Width = 1;
    uint32_t Height = 1;
    class World* World = nullptr;
    class CameraNode* Camera = nullptr;

    class UniformBuffer* SceneUniforms = nullptr;
    class ImageView* ShadowMap = nullptr;
    class Sampler* ShadowSampler = nullptr;
};

/** One modular piece of work inside a RenderStage, drawing into the stage's target. */
class RenderPass : public Object {
public:
    ARTIFACT_CLASS();

    virtual RenderCapability GetRequiredCapabilities() const { return RenderCapability::None; }

    /** False when the rendering API lacks a required capability. */
    bool IsSupported() const { return m_Supported; }
    bool IsEnabled() const { return m_Supported && m_Enabled; }
    void SetEnabled(bool InEnabled) { m_Enabled = InEnabled; }

    RenderStage* GetStage() const { return m_Stage; }
    String GetName() const { return GetClass().GetDisplayName(); }
    const RenderTiming& GetTiming() const { return m_Timer.GetTiming(); }

protected:
    /** Runs once the pass is attached and known to be supported. */
    virtual void Initialize() {}
    virtual void Execute(const RenderContext& InContext) = 0;

private:
    friend class RenderStage;
    void Attach(RenderStage& InStage);
    void Run(const RenderContext& InContext);

    RenderStage* m_Stage = nullptr;
    RenderTimer m_Timer;
    bool m_Enabled = true;
    bool m_Supported = false;
};