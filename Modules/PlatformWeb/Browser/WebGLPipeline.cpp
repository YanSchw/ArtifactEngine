#include "WebGLPipeline.h"

#include "ShaderTranslator.h"
#include "WebGLBuffer.h"
#include "WebGLImage.h"
#include "WebGLSampler.h"
#include "WebGLShader.h"

WebGLPipeline::WebGLPipeline(const PipelineDesc& InPipelineDesc)
    : m_Desc(InPipelineDesc) {
}

void WebGLPipeline::Apply(GLuint InShaderDataBuffer) {
    WebGLShader* shader = m_Desc.Shader ? m_Desc.Shader->As<WebGLShader>() : nullptr;
    if (!shader || shader->GetProgram() == 0) {
        return;
    }

    glUseProgram(shader->GetProgram());
    ApplyRenderState(shader->GetRenderState());

    glBindBufferBase(GL_UNIFORM_BUFFER, ShaderTranslator::ShaderDataBinding, InShaderDataBuffer);
    for (const SharedObjectPtr<ShaderBuffer>& buffer : m_Desc.Buffers) {
        WebGLUniformBuffer* uniformBuffer = buffer ? buffer->As<WebGLUniformBuffer>() : nullptr;
        if (uniformBuffer) {
            glBindBufferBase(GL_UNIFORM_BUFFER, uniformBuffer->GetBinding(), uniformBuffer->GetHandle());
        }
    }

    // Fragment shaders are only guaranteed 16 texture units, well below the binding slots the
    // engine hands out, so bindings are packed into consecutive units here.
    GLint unit = 0;
    for (const auto& [binding, view, sampler] : m_Desc.ImageBindings) {
        const GLint location = shader->GetSamplerLocation(binding);
        WebGLImageView* imageView = view ? view->As<WebGLImageView>() : nullptr;
        if (location < 0 || !imageView) {
            continue;
        }

        glActiveTexture(GL_TEXTURE0 + unit);
        imageView->Bind();
        glBindSampler((GLuint)unit, sampler ? sampler->As<WebGLSampler>()->GetHandle() : 0);
        glUniform1i(location, unit);
        unit++;
    }
}
