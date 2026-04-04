#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include "Core/Platform.h"
#include "Platform/Subprocess.h"
#include "Platform/FileIO.h"

static Array<VulkanPipeline*> s_Pipelines;

// FROM VulkanAPI.cpp - should be moved to a more appropriate place
extern VkExtent2D swapChainExtent;
extern VkFormat swapChainFormat;
extern VkSwapchainKHR oldSwapChain;
extern VkSwapchainKHR swapChain;
extern std::vector<VkImage> swapChainImages;
extern std::vector<VkImageView> swapChainImageViews;
extern std::vector<VkFramebuffer> swapChainFramebuffers;
extern VkRenderPass renderPass;

extern struct {
    glm::mat4 transformationMatrix;
} uniformBufferData;
extern VkBuffer uniformBuffer;

VulkanPipeline::VulkanPipeline(const PipelineDesc& InPipelineDesc, VulkanAPI& InVulkanAPI) {
    s_Pipelines.Add(this);
    m_Desc = InPipelineDesc;
    m_VulkanAPI = &InVulkanAPI;
    
    Invalidate();
}

VulkanPipeline::~VulkanPipeline() {
    s_Pipelines.Remove(this);
    Destroy();
}

void VulkanPipeline::Invalidate() {
    Destroy();

    AE_ASSERT(m_Desc.Shader && m_Desc.Shader->IsA<VulkanShader>(), "Pipeline must have a shader of type VulkanShader!");
    VulkanShader& vulkanShader = *m_Desc.Shader->As<VulkanShader>();
    AE_ASSERT(vulkanShader.IsVertexFragmentShader(), "shader must have vertex and fragment shader modules to be used in graphics pipeline");

    // Set up shader stage info
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.module = vulkanShader.m_VertexShaderModule;
    vertexShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.module = vulkanShader.m_FragmentShaderModule;
    fragmentShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    // Describe vertex input
    CreateVertexDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &m_VertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = m_Desc.VertexLayout.Size();
    vertexInputCreateInfo.pVertexAttributeDescriptions = m_VertexAttributeDescriptions.data();

    // Describe input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Describe viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = swapChainExtent.width;
    scissor.extent.height = swapChainExtent.height;

    // Note: scissor test is always enabled (although dynamic scissor is possible)
    // Number of viewports must match number of scissors
    VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;

    // Describe rasterization
    // Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;

    // Describe multisampling
    // Note: using multisampling also requires turning on device features
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    // Describing color blending
    // Note: all paramaters except blendEnable and colorWriteMask are irrelevant here
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // Note: all attachments must have the same values unless a device feature is enabled
    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    // Describe pipeline layout
    // Note: this describes the mapping between memory and shader resources (descriptor sets)
    // This is for uniform buffers and samplers
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
    descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCreateInfo.bindingCount = 1;
    descriptorLayoutCreateInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(m_VulkanAPI->GetDevice(), &descriptorLayoutCreateInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        AE_ERROR("failed to create descriptor layout");
        exit(1);
    } else {
        AE_INFO("created descriptor layout");
    }

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;

    if (vkCreatePipelineLayout(m_VulkanAPI->GetDevice(), &layoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        AE_ERROR("failed to create pipeline layout");
        exit(1);
    } else {
        AE_INFO("created pipeline layout");
    }

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.layout = m_PipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_VulkanAPI->GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        AE_ERROR("failed to create graphics pipeline");
        exit(1);
    } else {
        AE_INFO("created graphics pipeline");
    }

    CreateDescriptorSet();
}

void VulkanPipeline::CreateVertexDescriptions() {
    // attribute descriptions
    m_VertexAttributeDescriptions.clear();
    size_t offset = 0;
    for (size_t i = 0; i < m_Desc.VertexLayout.Size(); i++) {
        ShaderDataType type = m_Desc.VertexLayout[i];
        if (type == ShaderDataType::Float3) {
            VkVertexInputAttributeDescription attributeDescription = {};
            attributeDescription.binding = 0;
            attributeDescription.location = (uint32_t)i;
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription.offset = offset;
            m_VertexAttributeDescriptions.push_back(attributeDescription);
            offset += sizeof(float) * 3;
        } else {
            AE_ERROR("unsupported vertex attribute type in pipeline vertex layout");
            exit(1);
        }
    }

    // Binding
    m_VertexBindingDescription.binding = 0;
    m_VertexBindingDescription.stride = offset;
    m_VertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void VulkanPipeline::CreateDescriptorSet() {
    // There needs to be one descriptor set per binding point in the shader
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_VulkanAPI->GetDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_DescriptorSetLayout;

    if (vkAllocateDescriptorSets(m_VulkanAPI->GetDevice(), &allocInfo, &m_DescriptorSet) != VK_SUCCESS) {
        AE_ERROR("failed to create descriptor set");
        exit(1);
    } else {
        AE_INFO("created descriptor set");
    }

    // Update descriptor set with uniform binding
    VkDescriptorBufferInfo descriptorBufferInfo = {};
    descriptorBufferInfo.buffer = uniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(uniformBufferData);

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = m_DescriptorSet;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
    writeDescriptorSet.dstBinding = 0;

    vkUpdateDescriptorSets(m_VulkanAPI->GetDevice(), 1, &writeDescriptorSet, 0, nullptr);
}

PipelineDesc VulkanPipeline::GetDesc() const {
    return m_Desc;
}

void VulkanPipeline::Destroy() {
    if (m_Pipeline) {
        vkDestroyPipeline(m_VulkanAPI->GetDevice(), m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }
    if (m_PipelineLayout) {
        vkDestroyPipelineLayout(m_VulkanAPI->GetDevice(), m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
    if (m_DescriptorSet) {
        vkFreeDescriptorSets(m_VulkanAPI->GetDevice(), m_VulkanAPI->GetDescriptorPool(), 1, &m_DescriptorSet);
        m_DescriptorSet = VK_NULL_HANDLE;
    }
    if (m_DescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(m_VulkanAPI->GetDevice(), m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }
}

void VulkanPipeline::InvalidateAll() {
    Array<VulkanPipeline*> pipelinesToInvalidate = s_Pipelines;
    for (VulkanPipeline* pipeline : pipelinesToInvalidate) {
        pipeline->Invalidate();
    }
}

void VulkanPipeline::DestroyAll() {
    Array<VulkanPipeline*> pipelinesToDestroy = s_Pipelines;
    for (VulkanPipeline* pipeline : pipelinesToDestroy) {
        delete pipeline;
    }
    s_Pipelines.Clear();
}