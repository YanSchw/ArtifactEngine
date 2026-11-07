#include "RenderPipeline.h"
#include "FrameBuffer.h"
#include "Sampler.h"
#include "ShaderLibrary.h"
#include "Surface.h"
#include "Core/EngineConfig.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"

static constexpr uint32_t s_PresentBinding = 16;
static constexpr const char* s_PresentShader = "/Shaders/Passthrough.glsl";

SharedObjectPtr<RenderPipeline> RenderPipeline::Create() {
    Object* object = Object::Create(EngineConfig::RenderPipelineClass());
    SharedObjectPtr<RenderPipeline> pipeline = object ? object->As<RenderPipeline>() : nullptr;
    AE_ASSERT(pipeline, "Failed to create RenderPipeline instance!");

    pipeline->Initialize();
    AE_ASSERT(pipeline->GetFirstStage(), "a RenderPipeline has to define at least one RenderStage");
    return pipeline;
}

RenderStage* RenderPipeline::SetFirstStage(RenderStageName InName) {
    AE_ASSERT(!m_FirstStage, "the first stage is already set");
    m_FirstStage = RenderStage::Create(*this, InName);
    return m_FirstStage.Get();
}

RenderStage* RenderPipeline::GetFinalStage() const {
    RenderStage* stage = GetFirstStage();
    while (stage && stage->GetNext()) {
        stage = stage->GetNext();
    }
    return stage;
}

RenderStage* RenderPipeline::FindStage(RenderStageName InName) const {
    for (RenderStage* stage = GetFirstStage(); stage; stage = stage->GetNext()) {
        if (stage->GetName() == InName) {
            return stage;
        }
    }
    return nullptr;
}

SharedObjectPtr<FrameBuffer> RenderPipeline::GetFrameBuffer() const {
    RenderStage* stage = GetFinalStage();
    return stage ? stage->GetTarget() : nullptr;
}

SharedObjectPtr<ImageView> RenderPipeline::GetFinalImageView() const {
    FrameBuffer* target = GetFrameBuffer().Get();
    return target ? target->GetDesc().ColorAttachments[0] : nullptr;
}

void RenderPipeline::Render(double InDeltaTime, const RenderParams& InParams) {
    m_Timer.Begin();

    RenderContext context;
    context.DeltaTime = InDeltaTime;
    context.Width = glm::max(InParams.Width, 1u);
    context.Height = glm::max(InParams.Height, 1u);
    context.World = InParams.m_World;
    context.Camera = InParams.CameraOverride;
    if (!context.Camera && context.World) {
        context.Camera = context.World->GetMainCamera();
    }
    PrepareFrame(context);

    RenderStage* previous = nullptr;
    for (RenderStage* stage = GetFirstStage(); stage; stage = stage->GetNext()) {
        stage->m_Timer.Begin();
        TransitionRenderStage(previous, *stage, context);
        stage->RunPasses(context);
        stage->m_Timer.End();
        previous = stage;
    }

    if (InParams.PresentTarget) {
        Present(*InParams.PresentTarget);
    }

    m_Timer.End();
}

void RenderPipeline::Present(Surface& InSurface) {
    if (!m_PresentShader) {
        m_PresentShader = ShaderLibrary::CreateShader(s_PresentShader);
        m_PresentSampler = Sampler::Create({ FilterMode::Nearest, FilterMode::Nearest,
                                             AddressMode::Repeat, AddressMode::Repeat, AddressMode::Repeat });
    }

    PipelineDesc desc;
    desc.Target = &InSurface;
    desc.Shader = m_PresentShader;
    desc.ImageBindings.Add({ s_PresentBinding, GetFinalImageView(), m_PresentSampler });
    m_PresentQuad.Draw(desc);
}
