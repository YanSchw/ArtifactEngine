#pragma once
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanPipeline.gen.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>

class VulkanPipeline : public Pipeline {
public:
    ARTIFACT_CLASS();

    VulkanPipeline(const PipelineDesc& InPipelineDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanPipeline();
    virtual void Invalidate() override;
    virtual PipelineDesc GetDesc() const override;

    void CreateVertexDescriptions();
    void CreateDescriptorSet();
    void Destroy();

    static void InvalidateAll();
    static void DestroyAll();

private:
    PipelineDesc m_Desc;
    VulkanAPI* m_VulkanAPI = nullptr;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

    VkVertexInputBindingDescription m_VertexBindingDescription;
    std::vector<VkVertexInputAttributeDescription> m_VertexAttributeDescriptions;

    friend class VulkanAPI;
};