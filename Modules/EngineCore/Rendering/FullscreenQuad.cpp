#include "FullscreenQuad.h"
#include "VertexBuffer.h"
#include "Buffer.h"

FullscreenQuad::FullscreenQuad() {
    const Array<Vertex> vertices = {
        { { -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
        { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }
    };
    m_Quad = VertexBuffer::Create(vertices, { 0, 1, 2, 0, 2, 3 });
}

FullscreenQuad::~FullscreenQuad() = default;

Array<void*> FullscreenQuad::Identify(const PipelineDesc& InDesc) {
    Array<void*> identity = { InDesc.Target.Get(), InDesc.Shader.Get() };
    for (const SharedObjectPtr<ShaderBuffer>& buffer : InDesc.Buffers) {
        identity.Add(buffer.Get());
    }
    for (const auto& [binding, view, sampler] : InDesc.ImageBindings) {
        identity.Add((void*)(uintptr_t)binding);
        identity.Add(view.Get());
        identity.Add(sampler.Get());
    }
    return identity;
}

void FullscreenQuad::Draw(const PipelineDesc& InDesc) {
    Array<void*> identity = Identify(InDesc);
    if (!m_Pipeline || !(identity == m_Identity)) {
        m_Pipeline = Pipeline::Create(InDesc);
        m_Identity = identity;
    }
    m_Pipeline->Bind();
    m_Quad->Draw();
}
