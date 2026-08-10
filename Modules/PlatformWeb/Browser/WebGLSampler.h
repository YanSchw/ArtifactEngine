#pragma once
#include "Rendering/Sampler.h"
#include "WebGLCommon.h"
#include "WebGLSampler.gen.h"

class WebGLSampler : public Sampler {
public:
    ARTIFACT_CLASS();

    explicit WebGLSampler(const SamplerDesc& InSamplerDesc);
    virtual ~WebGLSampler();

    GLuint GetHandle() const { return m_Sampler; }

private:
    GLuint m_Sampler = 0;
};
