#include "VulkanSampler.h"
#include "VulkanImage.h"

static Array<VulkanSampler*> s_Samplers;

VkFilter FilterModeToVkFilter(FilterMode mode) {
    switch (mode) {
        case FilterMode::Nearest: return VK_FILTER_NEAREST;
        case FilterMode::Linear: return VK_FILTER_LINEAR;
        default: return VK_FILTER_LINEAR;
    }
}

VkSamplerAddressMode AddressModeToVkAddressMode(AddressMode mode) {
    switch (mode) {
        case AddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case AddressMode::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case AddressMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VulkanSampler::VulkanSampler(const SamplerDesc& InSamplerDesc, VulkanAPI& InVulkanAPI) {
    s_Samplers.Add(this);
    m_VulkanAPI = &InVulkanAPI;

    // Create Vulkan sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = FilterModeToVkFilter(InSamplerDesc.MagFilter);
    samplerInfo.minFilter = FilterModeToVkFilter(InSamplerDesc.MinFilter);
    samplerInfo.addressModeU = AddressModeToVkAddressMode(InSamplerDesc.AddressU);
    samplerInfo.addressModeV = AddressModeToVkAddressMode(InSamplerDesc.AddressV);
    samplerInfo.addressModeW = AddressModeToVkAddressMode(InSamplerDesc.AddressW);
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_VulkanAPI->GetDevice(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

VulkanSampler::~VulkanSampler() {
    s_Samplers.Remove(this);

    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_VulkanAPI->GetDevice(), m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }
}

void VulkanSampler::DestroyAll() {
    Array<VulkanSampler*> samplersToDestroy = s_Samplers;
    for (VulkanSampler* sampler : samplersToDestroy) {
        delete sampler;
    }
    s_Samplers.Clear();
}
