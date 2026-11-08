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
    if (!target || target->GetDesc().Width != scene.Width || target->GetDesc().Height != scene.Height) {
        if (target) {
            RenderingAPI::GetInstance()->WaitIdle();
        }
        FrameBufferDesc desc;
        desc.Width = scene.Width;
        desc.Height = scene.Height;
        desc.ColorAttachments.Add(ImageView::CreateAttachment(scene.Width, scene.Height, ImageFormat::RGBA8,
                                                              ImageUsage::ColorAttachment | ImageUsage::Sampled));
        desc.ClearColor = Vec4(0.0f);
        target = FrameBuffer::Create(desc);
    }
    return *target;
}

void PostProcessChain::Advance(FrameBuffer& InTarget) {
    m_Result = &InTarget;
}

FrameBuffer& PostProcessPass::AcquireTarget(const RenderContext& InContext, PostProcessChain& InChain) {
    (void)InContext;
    return InChain.AcquireTarget();
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
    FrameBuffer& target = AcquireTarget(InContext, *chain);
    target.Begin();
    Process(InContext, *source, target);
    chain->Advance(target);
}