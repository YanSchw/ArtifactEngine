#pragma once
#include "Rendering/RenderPass.h"
#include "Object/Pointer.h"
#include "UIRenderer.h"
#include <memory>

#include "UIRenderPass.gen.h"

class UICanvas;

/** Draws the canvases the World's UIWorld routed this frame, on top of the finished image. */
class UIRenderPass : public RenderPass {
public:
    ARTIFACT_CLASS();

    virtual RenderStageName GetStageName() const override { return RenderStageName::UI; }

protected:
    virtual void Execute(const RenderContext& InContext) override;

private:
    UIRenderer& GetRenderer(UICanvas& InCanvas);
    void ReleaseDeadCanvases();

    Array<WeakObjectPtr<UICanvas>> m_Canvases;
    Array<std::shared_ptr<UIRenderer>> m_Renderers;
};