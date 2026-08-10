#pragma once
#include "Rendering/Pipeline.h"
#include "WebGLCommon.h"
#include "WebGLPipeline.gen.h"

class WebGLPipeline : public Pipeline {
public:
    ARTIFACT_CLASS();

    explicit WebGLPipeline(const PipelineDesc& InPipelineDesc);

    // GL resolves its state at draw time, so there is nothing cached to rebuild.
    virtual void Invalidate() override { }
    virtual PipelineDesc GetDesc() const override { return m_Desc; }

    void Apply(GLuint InShaderDataBuffer);

private:
    PipelineDesc m_Desc;
};
