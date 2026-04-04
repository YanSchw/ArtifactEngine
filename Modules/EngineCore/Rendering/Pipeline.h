#pragma once
#include "CoreMinimal.h"
#include "Pipeline.gen.h"

struct PipelineDesc {
    SharedObjectPtr<class Shader> Shader;
};

class Pipeline : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~Pipeline() { }
    virtual void Invalidate() = 0;
    virtual PipelineDesc GetDesc() const = 0;

    void Bind();
    static void InvalidateAll();
    static SharedObjectPtr<Pipeline> Create(const PipelineDesc& InPipelineDesc);
};