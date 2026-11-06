#pragma once
#include "Rendering/PipelineResources.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>

/** The descriptor set, its layout and the pipeline layout that graphics and compute pipelines build
 *  from their PipelineResources. */
class VulkanPipelineLayout {
public:
    static constexpr uint32_t MaxShaderDataSize = 128;

    bool Create(VulkanAPI& InVulkanAPI, const PipelineResources& InResources, VkShaderStageFlags InStages);

    /** Hands this layout and the pipeline built on it over to FlushRetired, since the frame in flight
     *  may still be using them. */
    void Retire(VkPipeline InPipeline);
    static void FlushRetired();

    void BindDescriptorSet(VkCommandBuffer InCmdBuffer, VkPipelineBindPoint InBindPoint) const;
    void PushShaderData(VkCommandBuffer InCmdBuffer, const class ShaderData& InShaderData) const;

    VkPipelineLayout GetHandle() const { return m_PipelineLayout; }

private:
    VulkanAPI* m_VulkanAPI = nullptr;
    VkShaderStageFlags m_Stages = 0;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
};
