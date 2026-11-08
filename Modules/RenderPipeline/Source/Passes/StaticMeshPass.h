#pragma once
#include "Rendering/RenderPass.h"
#include "Rendering/ShaderSource.h"
#include "MaterialPipelineCache.h"
#include "StaticMeshPass.gen.h"

/** Draws every StaticMeshNode sub-mesh whose material renders on the pass's RenderPath. */
class StaticMeshPass : public RenderPass {
public:
    ARTIFACT_CLASS();

protected:
    virtual RenderPath GetRenderPath() const = 0;
    virtual void Execute(const RenderContext& InContext) override;

private:
    MaterialPipelineCache m_Pipelines;
};

class StaticMeshDeferredPass : public StaticMeshPass {
public:
    ARTIFACT_CLASS();

    virtual RenderStageName GetStageName() const override { return RenderStageName::DeferredGeometry; }

protected:
    virtual RenderPath GetRenderPath() const override { return RenderPath::Deferred; }
};

class StaticMeshForwardPass : public StaticMeshPass {
public:
    ARTIFACT_CLASS();

    virtual RenderStageName GetStageName() const override { return RenderStageName::Forward; }

protected:
    virtual RenderPath GetRenderPath() const override { return RenderPath::Forward; }
};
