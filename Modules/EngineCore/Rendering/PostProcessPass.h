#pragma once
#include "RenderPass.h"
#include "Image.h"
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

protected:
    /** Whether the pass has anything to do this frame; an idle pass leaves the image untouched. */
    virtual bool IsActive(const RenderContext& InContext) const { (void)InContext; return true; }
    /** Where Process draws to. A pass that keeps its output around, e.g. as history, supplies its own. */
    virtual FrameBuffer& AcquireTarget(const RenderContext& InContext, PostProcessChain& InChain);
    /** Draws InSource, processed, into InTarget, whose render pass has already begun. */
    virtual void Process(const RenderContext& InContext, ImageView& InSource, FrameBuffer& InTarget) = 0;

    bool IsRunning(const RenderContext& InContext) const;

private:
    virtual void Execute(const RenderContext& InContext) override final;
};