#include "RenderStage.h"
#include "RenderPipeline.h"
#include "FrameBuffer.h"

RenderStage* RenderStage::Create(RenderPipeline& InPipeline, RenderStageName InName) {
    AE_ASSERT(!InPipeline.FindStage(InName), "a pipeline cannot have two stages of the same name");
    RenderStage* stage = Object::Create<RenderStage>();
    stage->m_Pipeline = &InPipeline;
    stage->m_Name = InName;
    return stage;
}

String RenderStage::GetDisplayName() const {
    return Enum("RenderStageName").ConvertValueToString((int64_t)m_Name);
}

RenderStage* RenderStage::Then(RenderStageName InName) {
    RenderStage* stage = Create(*m_Pipeline, InName);
    stage->m_Next = m_Next;
    m_Next = stage;
    return stage;
}

void RenderStage::SetTarget(FrameBuffer* InTarget) {
    m_Target = InTarget;
}

void RenderStage::BeginTarget() const {
    AE_ASSERT(m_Target, "a stage needs a target before it can begin");
    m_Target->Begin();
}

void RenderStage::RunPasses(const RenderContext& InContext) {
    for (const SharedObjectPtr<RenderPass>& pass : m_Passes) {
        pass->Run(InContext);
    }
}

void RenderStage::AddPass(RenderPass* InPass) {
    AE_ASSERT(InPass && !InPass->GetStage(), "a pass can only be attached to one stage");
    int32_t index = m_Passes.Size();
    while (index > 0 && m_Passes[index - 1]->GetOrderPreferenceIndex() > InPass->GetOrderPreferenceIndex()) {
        index--;
    }
    m_Passes.Insert(index, InPass);
    InPass->Attach(*this);
}

void RenderStage::RemovePass(RenderPass* InPass) {
    for (int32_t i = 0; i < m_Passes.Size(); i++) {
        if (m_Passes[i].Get() == InPass) {
            InPass->m_Stage = nullptr;
            m_Passes.RemoveAt(i);
            return;
        }
    }
}