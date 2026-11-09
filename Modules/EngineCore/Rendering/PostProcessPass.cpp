#include "PostProcessPass.h"
#include "FrameBuffer.h"
#include "RenderingAPI.h"

void PostProcessChain::Begin(FrameBuffer& InScene) {
    m_Scene = &InScene;
    m_Result = &InScene;
}

ImageView* PostProcessChain::GetSource() const {
    return m_Result ? m_Result->GetDesc().ColorAttachments[0].Get() : nullptr;
}

FrameBuffer& PostProcessChain::AcquireTarget() {
    SharedObjectPtr<FrameBuffer>& target = m_Targets[0].Get() == m_Result ? m_Targets[1] : m_Targets[0];
    const FrameBufferDesc& scene = m_Scene->GetDesc();
    PostProcessPass::EnsureTarget(target, scene.Width, scene.Height, PostProcessPass::ChainFormat);
    return *target;
}

void PostProcessChain::Advance(FrameBuffer& InTarget) {
    m_Result = &InTarget;
}

FrameBuffer& PostProcessPass::AcquireTarget(const RenderContext& InContext, PostProcessChain& InChain) {
    (void)InContext;
    return InChain.AcquireTarget();
}

bool PostProcessPass::EnsureTarget(SharedObjectPtr<FrameBuffer>& InOutTarget, uint32_t InWidth, uint32_t InHeight, ImageFormat InFormat) {
    if (InOutTarget) {
        const FrameBufferDesc& current = InOutTarget->GetDesc();
        if (current.Width == InWidth && current.Height == InHeight
            && current.ColorAttachments[0]->GetDesc().Format == InFormat) {
            return false;
        }
        RenderingAPI::GetInstance()->WaitIdle();
    }
    FrameBufferDesc desc;
    desc.Width = InWidth;
    desc.Height = InHeight;
    desc.ColorAttachments.Add(ImageView::CreateAttachment(InWidth, InHeight, InFormat,
                                                          ImageUsage::ColorAttachment | ImageUsage::Sampled));
    desc.ClearColor = Vec4(0.0f);
    InOutTarget = FrameBuffer::Create(desc);
    return true;
}

bool PostProcessPass::IsRunning(const RenderContext& InContext) const {
    return InContext.PostProcessEnabled && IsActive(InContext);
}

void PostProcessPass::Execute(const RenderContext& InContext) {
    PostProcessChain* chain = InContext.PostChain;
    ImageView* source = chain ? chain->GetSource() : nullptr;
    if (!source || !IsRunning(InContext)) {
        return;
    }
    RenderIntermediates(InContext, *source);
    FrameBuffer& target = AcquireTarget(InContext, *chain);
    target.Begin();
    Process(InContext, *source, target);
    chain->Advance(target);
}
