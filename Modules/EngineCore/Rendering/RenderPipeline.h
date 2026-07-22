#pragma once
#include "CoreMinimal.h"
#include "RenderPipeline.gen.h"

struct RenderParams {
    uint32_t Width;
    uint32_t Height;
    class World* m_World = nullptr;
    class CameraNode* CameraOverride = nullptr;
};

class RenderPipeline : public Object {
public:
    ARTIFACT_CLASS();

    RenderPipeline() = default;

    virtual void Render(double InDeltaTime, const RenderParams& InParams) = 0;
    virtual SharedObjectPtr<class ImageView> GetFinalImageView() const = 0;
    virtual SharedObjectPtr<class FrameBuffer> GetFrameBuffer() const { return nullptr; }

    virtual uint32_t PickNodeId(uint32_t InX, uint32_t InY) const { (void)InX; (void)InY; return 0; }

};