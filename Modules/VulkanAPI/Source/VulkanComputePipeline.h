#pragma once
#include "Rendering/ComputePipeline.h"
#include "VulkanAPI.h"
#include "VulkanPipelineLayout.h"

#include <vulkan/vulkan.h>
#include "VulkanComputePipeline.gen.h"

class VulkanComputePipeline : public ComputePipeline {
public:
    ARTIFACT_CLASS();

    VulkanComputePipeline(const ComputePipelineDesc& InComputePipelineDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanComputePipeline();
    virtual void Invalidate() override;

    bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }
    VkPipeline GetHandle() const { return m_Pipeline; }
    const VulkanPipelineLayout& GetLayout() const { return m_Layout; }

    static void InvalidateAll();
    static void DestroyAll();

private:
    void Destroy();

    VulkanAPI* m_VulkanAPI = nullptr;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VulkanPipelineLayout m_Layout;
};
