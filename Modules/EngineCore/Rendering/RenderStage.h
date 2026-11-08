#pragma once
#include "CoreMinimal.h"
#include "RenderPass.h"
#include "RenderStage.gen.h"

class FrameBuffer;
class RenderPipeline;

/** A link in a RenderPipeline's chain of stages. Its passes run by their order preference index,
 *  in the order they were added among equal ones. */
class RenderStage : public Object {
public:
    ARTIFACT_CLASS();

    RenderStageName GetName() const { return m_Name; }
    String GetDisplayName() const;

    RenderPipeline* GetPipeline() const { return m_Pipeline; }
    RenderStage* GetNext() const { return m_Next.Get(); }
    /** Links a new stage directly behind this one and returns it. */
    RenderStage* Then(RenderStageName InName);

    FrameBuffer* GetTarget() const { return m_Target.Get(); }
    void SetTarget(FrameBuffer* InTarget);
    /** Starts a render pass on the target, which clears it. */
    void BeginTarget() const;

    template<typename T>
    T* AddPass() {
        T* pass = Object::Create<T>();
        AddPass(pass);
        return pass;
    }
    void AddPass(RenderPass* InPass);
    void RemovePass(RenderPass* InPass);
    const Array<SharedObjectPtr<RenderPass>>& GetPasses() const { return m_Passes; }

    template<typename T>
    T* FindPass() const {
        for (const SharedObjectPtr<RenderPass>& pass : m_Passes) {
            if (T* found = pass->As<T>()) {
                return found;
            }
        }
        return nullptr;
    }

    const RenderTiming& GetTiming() const { return m_Timer.GetTiming(); }

private:
    friend class RenderPipeline;
    static RenderStage* Create(RenderPipeline& InPipeline, RenderStageName InName);
    void RunPasses(const RenderContext& InContext);

    RenderStageName m_Name = RenderStageName::Forward;
    RenderPipeline* m_Pipeline = nullptr;
    SharedObjectPtr<RenderStage> m_Next;
    SharedObjectPtr<FrameBuffer> m_Target;
    Array<SharedObjectPtr<RenderPass>> m_Passes;
    RenderTimer m_Timer;
};