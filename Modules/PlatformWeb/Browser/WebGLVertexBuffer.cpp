#include "WebGLVertexBuffer.h"

WebGLVertexBuffer::WebGLVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize,
                                     const Array<uint32_t>& InIndices, bool InDynamic)
    : m_Usage(InDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW) {
    glGenVertexArrays(1, &m_VertexArray);
    glGenBuffers(1, &m_VertexBuffer);
    glGenBuffers(1, &m_IndexBuffer);
    Upload(InVertexData, InVertexByteSize, InIndices);
}

WebGLVertexBuffer::~WebGLVertexBuffer() {
    glDeleteBuffers(1, &m_VertexBuffer);
    glDeleteBuffers(1, &m_IndexBuffer);
    glDeleteVertexArrays(1, &m_VertexArray);
}

void WebGLVertexBuffer::Upload(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) {
    m_IndexCount = (uint32_t)InIndices.Size();

    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)InVertexByteSize, InVertexData, m_Usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(m_IndexCount * sizeof(uint32_t)),
                 InIndices.IsEmpty() ? nullptr : InIndices.Data(), m_Usage);
}

void WebGLVertexBuffer::Update(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) {
    Upload(InVertexData, InVertexByteSize, InIndices);
}

void WebGLVertexBuffer::Bind(const Array<ShaderDataType>& InLayout) {
    glBindVertexArray(m_VertexArray);
    if (m_Layout == InLayout) {
        return;
    }
    m_Layout = InLayout;

    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);

    GLsizei stride = 0;
    for (ShaderDataType type : InLayout) {
        stride += (GLsizei)GetTypeSize(type);
    }

    size_t offset = 0;
    for (int32_t location = 0; location < InLayout.Size(); location++) {
        const ShaderDataType type = InLayout[location];
        glEnableVertexAttribArray((GLuint)location);
        glVertexAttribPointer((GLuint)location, (GLint)GetComponentCount(type), GL_FLOAT, GL_FALSE,
                              stride, (const void*)offset);
        offset += GetTypeSize(type);
    }
}
