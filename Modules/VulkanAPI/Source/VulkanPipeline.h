#pragma once
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>
#include "VulkanPipeline.gen.h"

class VulkanPipeline : public Pipeline {
public:
    ARTIFACT_CLASS();

    VulkanPipeline(const PipelineDesc& InPipelineDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanPipeline();
    virtual void Invalidate() override;
    virtual PipelineDesc GetDesc() const override;

    void CreateVertexDescriptions();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSet();
    void Destroy();

    static void InvalidateAll();
    static void DestroyAll();

public:
    static const constexpr uint32_t MAX_SHADER_DATA_SIZE = 128;

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