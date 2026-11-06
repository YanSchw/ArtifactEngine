#include "VulkanPipelineLayout.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanSampler.h"

#include "Rendering/ShaderData.h"

#include <vector>

// Only one frame is buffered and its fence is waited at the top of the next Draw, so retired
// handles are safe to destroy the moment that fence is crossed.
struct RetiredPipeline {
    VkDevice Device;
    VkDescriptorPool DescriptorPool;
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;
    VkDescriptorSet DescriptorSet;
    VkDescriptorSetLayout DescriptorSetLayout;
};
static Array<RetiredPipeline> s_RetiredPipelines;

struct Descriptor {
    uint32_t Binding = 0;
    VkDescriptorType Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkDescriptorBufferInfo BufferInfo = {};
    VkDescriptorImageInfo ImageInfo = {};

    bool IsBuffer() const {
        return Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
};

static bool CollectDescriptors(const PipelineResources& InResources, std::vector<Descriptor>& OutDescriptors) {
    for (const SharedObjectPtr<ShaderBuffer>& buffer : InResources.Buffers) {
        Descriptor descriptor;
        if (VulkanUniformBuffer* uniformBuffer = buffer ? buffer->As<VulkanUniformBuffer>() : nullptr) {
            descriptor.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor.BufferInfo = uniformBuffer->GetDescriptorInfo();
        } else if (VulkanStorageBuffer* storageBuffer = buffer ? buffer->As<VulkanStorageBuffer>() : nullptr) {
            descriptor.Type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptor.BufferInfo = storageBuffer->GetDescriptorInfo();
        } else {
            AE_ERROR("pipeline resources hold a buffer that is not a Vulkan uniform or storage buffer");
            return false;
        }
        descriptor.Binding = buffer->GetBinding();
        OutDescriptors.push_back(descriptor);
    }

    for (const auto& [binding, view, sampler] : InResources.ImageBindings) {
        VulkanImageView* vkView = view ? view->As<VulkanImageView>() : nullptr;
        VulkanSampler* vkSampler = sampler ? sampler->As<VulkanSampler>() : nullptr;
        if (!vkView || !vkView->GetImage() || !vkSampler) {
            AE_ERROR("sampled image binding {0} needs a Vulkan image view and sampler", binding);
            return false;
        }

        Descriptor descriptor;
        descriptor.Binding = binding;
        descriptor.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor.ImageInfo = { vkSampler->GetVkSampler(), vkView->GetVkImageView(), vkView->GetImage()->GetShaderReadLayout() };
        OutDescriptors.push_back(descriptor);
    }

    for (const auto& [binding, view] : InResources.StorageImages) {
        VulkanImageView* vkView = view ? view->As<VulkanImageView>() : nullptr;
        if (!vkView || !vkView->GetImage() || !vkView->GetImage()->HasUsage(ImageUsage::Storage)) {
            AE_ERROR("storage image binding {0} needs an image created with ImageUsage::Storage", binding);
            return false;
        }

        Descriptor descriptor;
        descriptor.Binding = binding;
        descriptor.Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptor.ImageInfo = { VK_NULL_HANDLE, vkView->GetVkImageView(), VK_IMAGE_LAYOUT_GENERAL };
        OutDescriptors.push_back(descriptor);
    }

    return true;
}

bool VulkanPipelineLayout::Create(VulkanAPI& InVulkanAPI, const PipelineResources& InResources, VkShaderStageFlags InStages) {
    m_VulkanAPI = &InVulkanAPI;
    m_Stages = InStages;

    std::vector<Descriptor> descriptors;
    if (!CollectDescriptors(InResources, descriptors)) {
        return false;
    }

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    for (const Descriptor& descriptor : descriptors) {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = descriptor.Binding;
        layoutBinding.descriptorType = descriptor.Type;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = InStages;
        layoutBindings.push_back(layoutBinding);
    }

    const VkDevice device = InVulkanAPI.GetDevice();

    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = (uint32_t)layoutBindings.size();
    setLayoutInfo.pBindings = layoutBindings.data();
    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        AE_ERROR("failed to create descriptor set layout");
        return false;
    }

    const VkPushConstantRange pushConstantRange = { InStages, 0, MaxShaderDataSize };

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        AE_ERROR("failed to create pipeline layout");
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = InVulkanAPI.GetDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_DescriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet) != VK_SUCCESS) {
        AE_ERROR("failed to allocate descriptor set");
        return false;
    }

    std::vector<VkWriteDescriptorSet> writes;
    for (const Descriptor& descriptor : descriptors) {
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = descriptor.Binding;
        write.descriptorType = descriptor.Type;
        write.descriptorCount = 1;
        if (descriptor.IsBuffer()) {
            write.pBufferInfo = &descriptor.BufferInfo;
        } else {
            write.pImageInfo = &descriptor.ImageInfo;
        }
        writes.push_back(write);
    }
    vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);

    return true;
}

void VulkanPipelineLayout::Retire(VkPipeline InPipeline) {
    if (!InPipeline && !m_PipelineLayout && !m_DescriptorSet && !m_DescriptorSetLayout) {
        return;
    }

    s_RetiredPipelines.Add({
        m_VulkanAPI->GetDevice(),
        m_VulkanAPI->GetDescriptorPool(),
        InPipeline,
        m_PipelineLayout,
        m_DescriptorSet,
        m_DescriptorSetLayout,
    });

    m_PipelineLayout = VK_NULL_HANDLE;
    m_DescriptorSet = VK_NULL_HANDLE;
    m_DescriptorSetLayout = VK_NULL_HANDLE;
}

void VulkanPipelineLayout::FlushRetired() {
    for (const RetiredPipeline& retired : s_RetiredPipelines) {
        if (retired.Pipeline) {
            vkDestroyPipeline(retired.Device, retired.Pipeline, nullptr);
        }
        if (retired.PipelineLayout) {
            vkDestroyPipelineLayout(retired.Device, retired.PipelineLayout, nullptr);
        }
        if (retired.DescriptorSet) {
            VkDescriptorSet set = retired.DescriptorSet;
            vkFreeDescriptorSets(retired.Device, retired.DescriptorPool, 1, &set);
        }
        if (retired.DescriptorSetLayout) {
            vkDestroyDescriptorSetLayout(retired.Device, retired.DescriptorSetLayout, nullptr);
        }
    }
    s_RetiredPipelines.Clear();
}

void VulkanPipelineLayout::BindDescriptorSet(VkCommandBuffer InCmdBuffer, VkPipelineBindPoint InBindPoint) const {
    vkCmdBindDescriptorSets(InCmdBuffer, InBindPoint, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);
}

void VulkanPipelineLayout::PushShaderData(VkCommandBuffer InCmdBuffer, const ShaderData& InShaderData) const {
    if (InShaderData.Size() == 0) {
        return;
    }
    AE_ASSERT(InShaderData.Size() <= MaxShaderDataSize);
    vkCmdPushConstants(InCmdBuffer, m_PipelineLayout, m_Stages, 0, (uint32_t)InShaderData.Size(), InShaderData.Data());
}
