#include "UIRenderPass.h"
#include "UIDrawList.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UIWorld.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/RenderStage.h"
#include "Rendering/RenderingAPI.h"

void UIRenderPass::ReleaseDeadCanvases() {
    bool waited = false;
    for (int32_t i = m_Canvases.Size() - 1; i >= 0; i--) {
        if (m_Canvases[i].Get()) {
            continue;
        }
        if (!waited) {
            RenderingAPI::GetInstance()->WaitIdle();
            waited = true;
        }
        m_Canvases.RemoveAt(i);
        m_Renderers.RemoveAt(i);
    }
}

UIRenderer& UIRenderPass::GetRenderer(UICanvas& InCanvas) {
    for (int32_t i = 0; i < m_Canvases.Size(); i++) {
        if (m_Canvases[i].Get() == &InCanvas) {
            return *m_Renderers[i];
        }
    }
    m_Canvases.Add(&InCanvas);
    m_Renderers.Add(std::make_shared<UIRenderer>());
    return *m_Renderers[m_Renderers.Size() - 1];
}

void UIRenderPass::Execute(const RenderContext& InContext) {
    ReleaseDeadCanvases();
    UIWorld* ui = UIWorld::Get(InContext.World);
    if (!ui) {
        return;
    }
    const Vec2 viewportSize((float)InContext.Width, (float)InContext.Height);
    for (UICanvas* canvas : ui->ConsumeRoutedCanvases()) {
        UIDrawList drawList;
        const Mat4 projection = ui->Paint(*canvas, drawList);
        GetRenderer(*canvas).Submit(GetStage()->GetTarget(), viewportSize, drawList, projection);
    }
}
