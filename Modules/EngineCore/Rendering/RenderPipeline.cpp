#include "RenderPipeline.h"
#include "FrameBuffer.h"
#include "Sampler.h"
#include "ShaderLibrary.h"
#include "Surface.h"
#include "Core/EngineConfig.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/PostProcessVolumeNode.h"

#include <algorithm>

static constexpr uint32_t s_PresentBinding = 16;
static constexpr const char* s_PresentShader = "/Shaders/Passthrough.glsl";

SharedObjectPtr<RenderPipeline> RenderPipeline::Create() {
    Object* object = Object::Create(EngineConfig::RenderPipelineClass());
    SharedObjectPtr<RenderPipeline> pipeline = object ? object->As<RenderPipeline>() : nullptr;
    AE_ASSERT(pipeline, "Failed to create RenderPipeline instance!");

    pipeline->Initialize();
    AE_ASSERT(pipeline->GetFirstStage(), "a RenderPipeline has to define at least one RenderStage");
    pipeline->AttachPasses();
    return pipeline;
}

void RenderPipeline::AttachPasses() {
    Array<Class> passClasses = Class::GetSubclassesOf(RenderPass::StaticClass());
    std::sort(passClasses.begin(), passClasses.end(), [](const Class& InA, const Class& InB) { return InA.Name < InB.Name; });

    for (const Class& passClass : passClasses) {
        SharedObjectPtr<RenderPass> pass = Cast<RenderPass>(Object::Create(passClass));
        if (!pass) {
            continue;
        }
        RenderStage* stage = FindStage(pass->GetStageName());
        if (!stage) {
            continue;
        }
        if (!pass->CheckSupport()) {
            AE_WARN("{0} is left out: the rendering API lacks capabilities it requires", pass->GetName());
            continue;
        }
        stage->AddPass(pass.Get());
    }
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
    context.FrameIndex = m_FrameIndex++;
    context.Width = glm::max(InParams.Width, 1u);
    context.Height = glm::max(InParams.Height, 1u);
    context.World = InParams.m_World;
    context.Camera = InParams.CameraOverride;
    if (!context.Camera && context.World) {
        context.Camera = context.World->GetMainCamera();
    }
    context.PostProcess = PostProcessVolumeNode::Evaluate(context.World, context.Camera);
    context.PostProcessEnabled = !context.Camera || context.Camera->IsPostProcessingEnabled();

    for (RenderStage* stage = GetFirstStage(); stage; stage = stage->GetNext()) {
        for (const SharedObjectPtr<RenderPass>& pass : stage->GetPasses()) {
            if (pass->IsEnabled()) {
                pass->PrepareFrame(context);
            }
        }
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
