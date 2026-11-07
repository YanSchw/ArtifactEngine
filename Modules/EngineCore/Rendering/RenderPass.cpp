#include "RenderPass.h"
#include "RenderStage.h"

void RenderPass::Attach(RenderStage& InStage) {
    m_Stage = &InStage;
    m_Supported = RenderingAPI::GetInstance()->GetCapabilities().Supports(GetRequiredCapabilities());
    if (!m_Supported) {
        AE_WARN("{0} is disabled: the rendering API lacks capabilities it requires", GetName());
        return;
    }
    Initialize();
}

void RenderPass::Run(const RenderContext& InContext) {
    if (!IsEnabled()) {
        return;
    }
    m_Timer.Begin();
    Execute(InContext);
    m_Timer.End();
}