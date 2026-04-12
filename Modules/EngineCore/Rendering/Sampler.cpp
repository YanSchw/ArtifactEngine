#include "Sampler.h"
#include "RenderingAPI.h"

SharedObjectPtr<Sampler> Sampler::Create(const SamplerDesc& InSamplerDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateSampler(InSamplerDesc);
}