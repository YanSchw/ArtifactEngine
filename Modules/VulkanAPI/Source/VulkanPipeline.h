#pragma once
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanAPI.h"
#include "VulkanPipelineLayout.h"

#include <vulkan/vulkan.h>
#include "VulkanPipeline.gen.h"

class VulkanPipeline : public Pipeline {
public:
    ARTIFACT_CLASS();

    VulkanPipeline(const PipelineDesc& InPipelineDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanPipeline();
    virtual void Invalidate() override;

    bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }
    VkPipeline GetHandle() const { return m_Pipeline; }
    const VulkanPipelineLayout& GetLayout() const { return m_Layout; }

    static void InvalidateAll();
    static void DestroyAll();

private:
    void CreateVertexDescriptions();
    void Destroy();

    VulkanAPI* m_VulkanAPI = nullptr;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VulkanPipelineLayout m_Layout;

    std::vector<VkVertexInputBindingDescription> m_VertexBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> m_VertexAttributeDescriptions;
};
