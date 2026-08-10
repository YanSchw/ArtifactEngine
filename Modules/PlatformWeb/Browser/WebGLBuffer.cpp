#include "WebGLBuffer.h"

WebGLUniformBuffer::WebGLUniformBuffer(uint32_t InBinding, size_t InSize)
    : m_Binding(InBinding) {
    m_Staging.Resize(InSize);

    glGenBuffers(1, &m_Buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, m_Buffer);
    glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)InSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

WebGLUniformBuffer::~WebGLUniformBuffer() {
    if (m_Buffer != 0) {
        glDeleteBuffers(1, &m_Buffer);
    }
}

void* WebGLUniformBuffer::MapData(size_t InSize, size_t InOffset) {
    if (InOffset + InSize > (size_t)m_Staging.Size()) {
        AE_ERROR("Uniform buffer write of {0} bytes at {1} exceeds its size", InSize, InOffset);
        return nullptr;
    }

    m_MappedOffset = InOffset;
    m_MappedSize = InSize;
    return &m_Staging[(int32_t)InOffset];
}

void WebGLUniformBuffer::UnmapData() {
    if (m_MappedSize == 0) {
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, m_Buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)m_MappedOffset, (GLsizeiptr)m_MappedSize,
                    &m_Staging[(int32_t)m_MappedOffset]);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    m_MappedSize = 0;
}
