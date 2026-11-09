#pragma once
#include "Rendering/PostProcessPass.h"
#include "Rendering/FullscreenQuad.h"
#include "ScreenEffectPass.gen.h"

class Sampler;
class Shader;

/** A PostProcess pass that is a single fullscreen draw: one shader reading the image (binding 16)
 *  and one uniform block (binding 1). */
class ScreenEffectPass : public PostProcessPass {
public:
    ARTIFACT_CLASS();

protected:
    virtual const char* GetShaderPath() const = 0;
    virtual void Initialize() override;

    bool HasShader() const { return m_Shader.Get() != nullptr; }

    template<typename T>
    void DrawEffect(ImageView& InSource, FrameBuffer& InTarget, const T& InUniforms) {
        if (!m_Uniforms) {
            m_Uniforms = UniformBuffer::Create(1, sizeof(T));
        }
        Upload(*m_Uniforms, InUniforms);
        DrawEffect(InSource, InTarget);
    }

private:
    void DrawEffect(ImageView& InSource, FrameBuffer& InTarget);

    SharedObjectPtr<Shader> m_Shader;
    SharedObjectPtr<Sampler> m_LinearSampler;
    SharedObjectPtr<UniformBuffer> m_Uniforms;
    FullscreenQuad m_Quad;
};
