#pragma once
#include "Rendering/RenderPipeline.h"
#include "ArtifactRenderPipeline.gen.h"

class ArtifactRenderPipeline : public RenderPipeline {
public:
    ARTIFACT_CLASS();

    ArtifactRenderPipeline();
    ~ArtifactRenderPipeline();

    void Invalidate(uint32_t InWidth, uint32_t InHeight);

    virtual void Render(double InDeltaTime, const RenderParams& InParams) override;
    virtual SharedObjectPtr<class ImageView> GetFinalImageView() const override;
    
};