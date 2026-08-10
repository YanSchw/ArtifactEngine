#include "WebGLShader.h"

static String GetInfoLog(GLuint InObject, bool InIsProgram) {
    GLint length = 0;
    if (InIsProgram) {
        glGetProgramiv(InObject, GL_INFO_LOG_LENGTH, &length);
    } else {
        glGetShaderiv(InObject, GL_INFO_LOG_LENGTH, &length);
    }
    if (length <= 0) {
        return "";
    }

    String log;
    log.resize((size_t)length);
    if (InIsProgram) {
        glGetProgramInfoLog(InObject, length, nullptr, &log[0]);
    } else {
        glGetShaderInfoLog(InObject, length, nullptr, &log[0]);
    }
    return log;
}

static GLuint CompileStage(const CompiledShaderStage& InStage) {
    const GLuint shader = glCreateShader(InStage.Stage == ShaderStage::Vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    const GLchar* source = (const GLchar*)InStage.ByteCode->GetData();
    const GLint length = (GLint)InStage.ByteCode->GetSizeInBytes();
    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    AE_ERROR("Failed to compile the {0} stage:\n{1}", ShaderSource::GetStageName(InStage.Stage), GetInfoLog(shader, false));
    glDeleteShader(shader);
    return 0;
}

WebGLShader::WebGLShader(const CompiledShader& InCompiledShader) {
    m_RenderState = InCompiledShader.RenderState;
    Link(InCompiledShader);
}

WebGLShader::~WebGLShader() {
    Destroy();
}

void WebGLShader::Reload(const CompiledShader& InCompiledShader) {
    Destroy();
    m_RenderState = InCompiledShader.RenderState;
    Link(InCompiledShader);
}

void WebGLShader::Destroy() {
    if (m_Program != 0) {
        glDeleteProgram(m_Program);
        m_Program = 0;
    }
    m_SamplerLocations.Clear();
}

void WebGLShader::Link(const CompiledShader& InCompiledShader) {
    Array<GLuint> stages;
    for (const CompiledShaderStage& stage : InCompiledShader.Stages) {
        const GLuint compiled = CompileStage(stage);
        if (compiled == 0) {
            for (GLuint created : stages) {
                glDeleteShader(created);
            }
            return;
        }
        stages.Add(compiled);
    }

    m_Program = glCreateProgram();
    for (GLuint stage : stages) {
        glAttachShader(m_Program, stage);
    }
    glLinkProgram(m_Program);

    for (GLuint stage : stages) {
        glDetachShader(m_Program, stage);
        glDeleteShader(stage);
    }

    GLint linked = GL_FALSE;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        AE_ERROR("Failed to link the shader program:\n{0}", GetInfoLog(m_Program, true));
        Destroy();
        return;
    }

    // GLSL ES 3.00 cannot spell out resource slots, so the cooked binding table is replayed here.
    for (const CompiledShaderBinding& block : InCompiledShader.UniformBlocks) {
        const GLuint index = glGetUniformBlockIndex(m_Program, block.Name.c_str());
        if (index != GL_INVALID_INDEX) {
            glUniformBlockBinding(m_Program, index, block.Binding);
        }
    }
    for (const CompiledShaderBinding& sampler : InCompiledShader.Samplers) {
        const GLint location = glGetUniformLocation(m_Program, sampler.Name.c_str());
        if (location >= 0) {
            m_SamplerLocations[sampler.Binding] = location;
        }
    }
}

GLint WebGLShader::GetSamplerLocation(uint32_t InBinding) const {
    return m_SamplerLocations.ContainsKey(InBinding) ? m_SamplerLocations.At(InBinding) : -1;
}
