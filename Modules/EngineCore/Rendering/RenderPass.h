#pragma once
#include "CoreMinimal.h"
#include "RenderingAPI.h"
#include "RenderTiming.h"
#include "GameFramework/PostProcessParams.h"
#include "RenderPass.gen.h"

class RenderStage;
class PostProcessChain;

ARTIFACT_ENUM();
enum class RenderStageName : uint32_t {
    DeferredGeometry,
    DeferredLighting,
    Forward,
    PostProcess,
    UI
};

enum class RenderBufferKind : uint8_t {
    Color,
    Depth,
    NodeId
};

/** One attachment of a stage's target, shown in place of the finished image. */
struct RenderBufferView {
    static constexpr int32_t DepthAttachment = -1;

    String Label;
    RenderStageName Stage = RenderStageName::Forward;
    /** A color attachment index, or DepthAttachment. */
    int32_t Attachment = 0;
    RenderBufferKind Kind = RenderBufferKind::Color;
};

/** What every stage and pass of one RenderPipeline::Render call works from. */
struct RenderContext {
    double DeltaTime = 0.0;
    uint64_t FrameIndex = 0;
    uint32_t Width = 1;
    uint32_t Height = 1;
    class World* World = nullptr;
    class CameraNode* Camera = nullptr;
    /** Rendering stops after this view's stage and the final stage shows it instead. */
    const RenderBufferView* BufferView = nullptr;

    class UniformBuffer* SceneUniforms = nullptr;
    class ImageView* ShadowMap = nullptr;
    class Sampler* ShadowSampler = nullptr;

    /** The world's PostProcessVolumeNodes, already blended for the camera. */
    PostProcessParams PostProcess;
    /** False when the camera opted out of post processing. */
    bool PostProcessEnabled = true;
    /** Sub-pixel offset in NDC the scene's projection is shifted by this frame. */
    Vec2 ProjectionJitter = Vec2(0.0f);
    /** Set while the PostProcess stage runs. */
    PostProcessChain* PostChain = nullptr;
};

/** One modular piece of work inside a RenderStage, drawing into the stage's target.
 *  Every spawnable subclass attaches itself to each RenderPipeline that has its stage. */
class RenderPass : public Object {
public:
    ARTIFACT_CLASS();

    virtual RenderStageName GetStageName() const = 0;
    /** Passes of one stage run in ascending order. */
    virtual int32_t GetOrderPreferenceIndex() const { return 0; }
    virtual RenderCapability GetRequiredCapabilities() const { return RenderCapability::None; }

    /** Whether the rendering API offers every capability the pass requires. */
    bool CheckSupport() const;
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
    /** Runs for every enabled pass before the first stage, e.g. to jitter the projection. */
    virtual void PrepareFrame(RenderContext& InOutContext) { (void)InOutContext; }
    virtual void Execute(const RenderContext& InContext) = 0;

private:
    friend class RenderStage;
    friend class RenderPipeline;
    void Attach(RenderStage& InStage);
    void Run(const RenderContext& InContext);

    RenderStage* m_Stage = nullptr;
    RenderTimer m_Timer;
    bool m_Enabled = true;
    bool m_Supported = false;
};
