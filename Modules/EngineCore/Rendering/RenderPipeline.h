#pragma once
#include "CoreMinimal.h"
#include "RenderStage.h"
#include "FullscreenQuad.h"
#include "RenderPipeline.gen.h"

class FrameBuffer;
class ImageView;
class Sampler;
class Surface;

struct RenderParams {
    uint32_t Width;
    uint32_t Height;
    class World* m_World = nullptr;
    class CameraNode* CameraOverride = nullptr;
    /** Receives the final stage's image. Without one it stays in GetFinalImageView(). */
    Surface* PresentTarget = nullptr;
};

/** Drives a chain of RenderStages, each running the RenderPasses attached to it. */
class RenderPipeline : public Object {
public:
    ARTIFACT_CLASS();

    /** Creates the pipeline class the engine is configured with and has it define its stages. */
    static SharedObjectPtr<RenderPipeline> Create();

    void Render(double InDeltaTime, const RenderParams& InParams);

    RenderStage* GetFirstStage() const { return m_FirstStage.Get(); }
    RenderStage* GetFinalStage() const;
    RenderStage* FindStage(RenderStageName InName) const;

    template<typename T>
    T* FindPass() const {
        for (RenderStage* stage = GetFirstStage(); stage; stage = stage->GetNext()) {
            if (T* pass = stage->FindPass<T>()) {
                return pass;
            }
        }
        return nullptr;
    }

    const RenderTiming& GetTiming() const { return m_Timer.GetTiming(); }

    SharedObjectPtr<FrameBuffer> GetFrameBuffer() const;
    SharedObjectPtr<ImageView> GetFinalImageView() const;
    virtual uint32_t PickNodeId(uint32_t InX, uint32_t InY) const { (void)InX; (void)InY; return 0; }

protected:
    /** Defines the stages. There has to be at least one. */
    virtual void Initialize() = 0;
    /** Fills in what the stages share before the first one runs. */
    virtual void PrepareFrame(RenderContext& InOutContext) { (void)InOutContext; }
    /** Readies InTo's target for its passes. InFrom is null for the first stage. */
    virtual void TransitionRenderStage(RenderStage* InFrom, RenderStage& InTo, const RenderContext& InContext) = 0;

    RenderStage* SetFirstStage(RenderStageName InName);

private:
    void AttachPasses();
    void Present(Surface& InSurface);

    SharedObjectPtr<RenderStage> m_FirstStage;
    SharedObjectPtr<class Shader> m_PresentShader;
    SharedObjectPtr<Sampler> m_PresentSampler;
    FullscreenQuad m_PresentQuad;
    RenderTimer m_Timer;
    uint64_t m_FrameIndex = 0;
};
