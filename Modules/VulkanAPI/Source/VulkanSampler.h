#pragma once
#include "Rendering/Sampler.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>
#include "VulkanSampler.gen.h"

class VulkanSampler : public Sampler {
public:
    ARTIFACT_CLASS();

    VulkanSampler(const SamplerDesc& InSamplerDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanSampler();

    VkSampler GetVkSampler() const { return m_Sampler; }

    static void DestroyAll();

private:
    VulkanAPI* m_VulkanAPI = nullptr;
    VkSampler m_Sampler = VK_NULL_HANDLE;
};