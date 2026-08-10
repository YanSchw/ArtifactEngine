#include "WebGLSampler.h"

WebGLSampler::WebGLSampler(const SamplerDesc& InSamplerDesc) {
    glGenSamplers(1, &m_Sampler);
    glSamplerParameteri(m_Sampler, GL_TEXTURE_MIN_FILTER, GetWebGLMinFilter(InSamplerDesc.MinFilter, true));
    glSamplerParameteri(m_Sampler, GL_TEXTURE_MAG_FILTER, GetWebGLMagFilter(InSamplerDesc.MagFilter));
    glSamplerParameteri(m_Sampler, GL_TEXTURE_WRAP_S, GetWebGLAddressMode(InSamplerDesc.AddressU));
    glSamplerParameteri(m_Sampler, GL_TEXTURE_WRAP_T, GetWebGLAddressMode(InSamplerDesc.AddressV));
    glSamplerParameteri(m_Sampler, GL_TEXTURE_WRAP_R, GetWebGLAddressMode(InSamplerDesc.AddressW));

    if (InSamplerDesc.Compare != CompareOp::None) {
        glSamplerParameteri(m_Sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(m_Sampler, GL_TEXTURE_COMPARE_FUNC, GetWebGLCompareOp(InSamplerDesc.Compare));
    }
}

WebGLSampler::~WebGLSampler() {
    if (m_Sampler != 0) {
        glDeleteSamplers(1, &m_Sampler);
    }
}
