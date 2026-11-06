#include "VulkanComputePipeline.h"
#include "VulkanShader.h"

static Array<VulkanComputePipeline*> s_ComputePipelines;

VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineDesc& InComputePipelineDesc, VulkanAPI& InVulkanAPI) {
    s_ComputePipelines.Add(this);
    m_Desc = InComputePipelineDesc;
    m_VulkanAPI = &InVulkanAPI;

    Invalidate();
}

VulkanComputePipeline::~VulkanComputePipeline() {
    s_ComputePipelines.Remove(this);
    Destroy();
}

void VulkanComputePipeline::Invalidate() {
    Destroy();

    VulkanShader* shader = m_Desc.Shader ? m_Desc.Shader->As<VulkanShader>() : nullptr;
    const VkShaderModule computeModule = shader ? shader->GetModule(ShaderStage::Compute) : VK_NULL_HANDLE;
    if (computeModule == VK_NULL_HANDLE) {
        AE_ERROR("a compute pipeline needs a Vulkan shader with a compute stage");
        return;
    }

    if (!m_Layout.Create(*m_VulkanAPI, m_Desc, VK_SHADER_STAGE_COMPUTE_BIT)) {
        return;
    }

    VkComputePipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    createInfo.stage.module = computeModule;
    createInfo.stage.pName = "main";
    createInfo.layout = m_Layout.GetHandle();

    if (vkCreateComputePipelines(m_VulkanAPI->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        AE_ERROR("failed to create compute pipeline");
        m_Pipeline = VK_NULL_HANDLE;
    }
}

void VulkanComputePipeline::Destroy() {
    m_Layout.Retire(m_Pipeline);
    m_Pipeline = VK_NULL_HANDLE;
}

void VulkanComputePipeline::InvalidateAll() {
    Array<VulkanComputePipeline*> pipelinesToInvalidate = s_ComputePipelines;
    for (VulkanComputePipeline* pipeline : pipelinesToInvalidate) {
        pipeline->Invalidate();
    }
}

void VulkanComputePipeline::DestroyAll() {
    Array<VulkanComputePipeline*> pipelinesToDestroy = s_ComputePipelines;
    for (VulkanComputePipeline* pipeline : pipelinesToDestroy) {
        delete pipeline;
    }
    s_ComputePipelines.Clear();
}
