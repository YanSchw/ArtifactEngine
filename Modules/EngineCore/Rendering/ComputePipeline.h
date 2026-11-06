#pragma once
#include "CoreMinimal.h"
#include "PipelineResources.h"

#include "ComputePipeline.gen.h"

struct ComputePipelineDesc : public PipelineResources {
};

/** Dispatches run in submission order with the rest of the frame. */
class ComputePipeline : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~ComputePipeline() { }
    virtual void Invalidate() = 0;

    const ComputePipelineDesc& GetDesc() const { return m_Desc; }

    void Bind();
    /** Binds this pipeline first if another one is bound. */
    void Dispatch(uint32_t InGroupCountX, uint32_t InGroupCountY = 1, uint32_t InGroupCountZ = 1);
    void Execute(uint32_t InGroupCountX, uint32_t InGroupCountY = 1, uint32_t InGroupCountZ = 1);

    static SharedObjectPtr<ComputePipeline> Create(const ComputePipelineDesc& InComputePipelineDesc);

protected:
    ComputePipelineDesc m_Desc;
};