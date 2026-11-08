#pragma once
#include "Rendering/RenderPass.h"
#include "DebugDrawPass.gen.h"

/** The lines queued through DebugDraw, depth tested against the stage's target. */
class DebugDrawPass : public RenderPass {
public:
    ARTIFACT_CLASS();

    virtual RenderStageName GetStageName() const override { return RenderStageName::Forward; }

protected:
    virtual void Execute(const RenderContext& InContext) override;
};
