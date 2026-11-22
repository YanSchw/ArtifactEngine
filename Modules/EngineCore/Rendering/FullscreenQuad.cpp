#include "FullscreenQuad.h"
#include "VertexBuffer.h"
#include "Buffer.h"
#include "Shader.h"
#include "Image.h"

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
    bool current = m_Pipeline && identity == m_Identity;
    for (const WeakObjectPtr<Object>& object : m_Objects) {
        current = current && object.Get();
    }
    if (!current) {
        m_Pipeline = Pipeline::Create(InDesc);
        m_Identity = identity;
        m_Objects.Clear();
        const auto track = [this](Object* InObject) {
            if (InObject) {
                m_Objects.Add(WeakObjectPtr<Object>(InObject));
            }
        };
        track(InDesc.Target.Get());
        track(InDesc.Shader.Get());
        for (const auto& [binding, view, sampler] : InDesc.ImageBindings) {
            track(view.Get());
        }
    }
    m_Pipeline->Bind();
    m_Quad->Draw();
}
