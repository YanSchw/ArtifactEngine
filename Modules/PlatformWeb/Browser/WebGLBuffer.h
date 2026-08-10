#pragma once
#include "Rendering/Buffer.h"
#include "WebGLCommon.h"
#include "WebGLBuffer.gen.h"

/** WebGL cannot map GPU memory, so writes land in a staging copy and are flushed on unmap. */
class WebGLUniformBuffer : public UniformBuffer {
public:
    ARTIFACT_CLASS();

    WebGLUniformBuffer(uint32_t InBinding, size_t InSize);
    virtual ~WebGLUniformBuffer();

    virtual void* MapData(size_t InSize, size_t InOffset = 0) override;
    virtual void UnmapData() override;

    uint32_t GetBinding() const { return m_Binding; }
    GLuint GetHandle() const { return m_Buffer; }

private:
    GLuint m_Buffer = 0;
    uint32_t m_Binding = 0;
    Array<byte> m_Staging;
    size_t m_MappedOffset = 0;
    size_t m_MappedSize = 0;
};
