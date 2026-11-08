#pragma once
#include "RenderPass.h"
#include "Image.h"
#include "Buffer.h"
#include <cstring>
#include "PostProcessPass.gen.h"

class FrameBuffer;

/** The images the PostProcess stage works through. */
class PostProcessChain {
public:
    void Begin(FrameBuffer& InScene);

    FrameBuffer& GetScene() const { return *m_Scene; }
    FrameBuffer& GetResult() const { return *m_Result; }
    ImageView* GetSource() const;
    bool HasProcessed() const { return m_Result != m_Scene; }

    /** A scene-sized target the chain owns that does not hold the current result. */
    FrameBuffer& AcquireTarget();
    /** Makes InTarget, which a pass just drew into, the next pass's source. */
    void Advance(FrameBuffer& InTarget);

private:
    FrameBuffer* m_Scene = nullptr;
    FrameBuffer* m_Result = nullptr;
    SharedObjectPtr<FrameBuffer> m_Targets[2];
};

/** A pass of the PostProcess stage. It stays idle while the camera opted out of post processing. */
class PostProcessPass : public RenderPass {
public:
    ARTIFACT_CLASS();

    virtual RenderStageName GetStageName() const override { return RenderStageName::PostProcess; }

    static constexpr ImageFormat ChainFormat = ImageFormat::RGBA16F;

    /** Keeps InOutTarget a single color target of the given size and format, recreating it when either changed.
     *  Returns true when it had to be (re)created. */
    static bool EnsureTarget(SharedObjectPtr<FrameBuffer>& InOutTarget, uint32_t InWidth, uint32_t InHeight, ImageFormat InFormat);

protected:
    /** Whether the pass has anything to do this frame; an idle pass leaves the image untouched. */
    virtual bool IsActive(const RenderContext& InContext) const { (void)InContext; return true; }
    /** Where Process draws to. A pass that keeps its output around, e.g. as history, supplies its own. */
    virtual FrameBuffer& AcquireTarget(const RenderContext& InContext, PostProcessChain& InChain);
    /** Draws the intermediate images Process reads besides InSource, before InTarget's render pass begins. */
    virtual void RenderIntermediates(const RenderContext& InContext, ImageView& InSource) { (void)InContext; (void)InSource; }
    /** Draws InSource, processed, into InTarget, whose render pass has already begun. */
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) = 0;

    bool IsRunning(const RenderContext& InContext) const;

    template<typename T>
    static void Upload(UniformBuffer& InBuffer, const T& InData) {
        void* mapped = InBuffer.MapData(sizeof(T), 0);
        memcpy(mapped, &InData, sizeof(T));
        InBuffer.UnmapData();
    }

private:
    virtual void Execute(const RenderContext& InContext) override final;
};