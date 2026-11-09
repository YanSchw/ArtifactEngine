#pragma once
#include "Rendering/VertexBuffer.h"
#include "WebGLCommon.h"
#include "WebGLVertexBuffer.gen.h"

class WebGLVertexBuffer : public VertexBuffer {
public:
    ARTIFACT_CLASS();

    WebGLVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices, bool InDynamic);
    virtual ~WebGLVertexBuffer();

    virtual uint32_t GetIndexCount() const override { return m_IndexCount; }
    virtual void Update(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) override;

    /** Binds the vertex array, rebuilding its attribute setup when the layout changes. */
    void Bind(const Array<ShaderDataType>& InLayout);
    /** Points the attributes after InFirstLocation at this buffer, advancing once per instance. Returns the locations used. */
    uint32_t BindAsInstances(const Array<ShaderDataType>& InLayout, uint32_t InFirstLocation, uint32_t InFirstInstance);

private:
    void Upload(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices);

    GLuint m_VertexArray = 0;
    GLuint m_VertexBuffer = 0;
    GLuint m_IndexBuffer = 0;
    GLenum m_Usage = GL_STATIC_DRAW;
    uint32_t m_IndexCount = 0;
    Array<ShaderDataType> m_Layout;
};
