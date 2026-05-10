#pragma once
#include "CoreMinimal.h"
#include "RenderPipeline.gen.h"

struct RenderParams {
    uint32_t Width;
    uint32_t Height;
};

class RenderPipeline : public Object {
public:
    ARTIFACT_CLASS();

    RenderPipeline() = default;

    virtual void Render(double InDeltaTime, const RenderParams& InParams) = 0;
    virtual SharedObjectPtr<class ImageView> GetFinalImageView() const = 0;
    
};