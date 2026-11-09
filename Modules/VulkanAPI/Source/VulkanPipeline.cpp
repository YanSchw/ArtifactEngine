#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include "VulkanFrameBuffer.h"
#include "Helpers.h"

static Array<VulkanPipeline*> s_Pipelines;

extern VkExtent2D swapChainExtent;
extern VkFormat swapChainFormat;
extern VkSampleCountFlagBits swapChainSampleCount;

static VkCullModeFlags ToVulkanCullMode(CullMode InCullMode) {
    switch (InCullMode) {
        case CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
        case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case CullMode::None:  return VK_CULL_MODE_NONE;
    }
    return VK_CULL_MODE_BACK_BIT;
}

static void ApplyBlendMode(BlendMode InBlendMode, VkPipelineColorBlendAttachmentState& OutState) {
    OutState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    OutState.colorBlendOp = VK_BLEND_OP_ADD;
    OutState.alphaBlendOp = VK_BLEND_OP_ADD;
    OutState.blendEnable = InBlendMode == BlendMode::Opaque ? VK_FALSE : VK_TRUE;

    switch (InBlendMode) {
        case BlendMode::Opaque:
            OutState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            OutState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            OutState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            OutState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            break;
        case BlendMode::Alpha:
            OutState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            OutState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            OutState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            OutState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
        case BlendMode::Additive:
            OutState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            OutState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            OutState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            OutState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            break;
        case BlendMode::Multiply:
            OutState.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            OutState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            OutState.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            OutState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            break;
        case BlendMode::Premultiplied:
            OutState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            OutState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            OutState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            OutState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
    }
}

static VkExtent2D GetPipelineTargetExtent(const PipelineDesc& InDesc) {
    if (InDesc.IsFrameBufferTarget()) {
        const auto& frameBuffer = InDesc.Target->As<FrameBuffer>();
        return { frameBuffer->GetDesc().Width, frameBuffer->GetDesc().Height };
    } else if (InDesc.IsSurfaceTarget()) {
        return swapChainExtent;
    } else {
        AE_ASSERT(false, "Pipeline target must be either a FrameBuffer or a Surface");
        return {0, 0};
    }
}

static std::vector<VkFormat> GetPipelineTargetColorFormats(const PipelineDesc& InDesc) {
    if (InDesc.IsFrameBufferTarget()) {
        const auto& attachments = InDesc.Target->As<FrameBuffer>()->GetDesc().ColorAttachments;
        std::vector<VkFormat> formats;
        for (const auto& attachment : attachments) {
            formats.push_back(VulkanHelpers::ImageFormatToVkFormat(attachment->GetDesc().Format));
        }
        return formats;
    }
    return { swapChainFormat };
}

static VkSampleCountFlagBits GetPipelineTargetSampleCount(const PipelineDesc& InDesc) {
    if (InDesc.IsFrameBufferTarget()) {
        return VulkanHelpers::SampleCountToVkSampleCount(InDesc.Target->As<FrameBuffer>()->GetSamples());
    }
    return InDesc.IsSurfaceTarget() ? swapChainSampleCount : VK_SAMPLE_COUNT_1_BIT;
}

static VkFormat GetPipelineTargetDepthFormat(const PipelineDesc& InDesc) {
    if (InDesc.IsFrameBufferTarget() && InDesc.Target->As<FrameBuffer>()->GetDesc().DepthAttachment) {
        return VulkanHelpers::ImageFormatToVkFormat(InDesc.Target->As<FrameBuffer>()->GetDesc().DepthAttachment->GetDesc().Format);
    }
    return VK_FORMAT_UNDEFINED;
}

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
    if (!vulkanShader.IsVertexFragmentShader()) {
        AE_ERROR("a graphics pipeline needs a shader with vertex and fragment stages");
        return;
    }

    const ShaderRenderState& renderState = vulkanShader.GetRenderState();

    // Set up shader stage info
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.module = vulkanShader.GetModule(ShaderStage::Vertex);
    vertexShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.module = vulkanShader.GetModule(ShaderStage::Fragment);
    fragmentShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    // Describe vertex input
    CreateVertexDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = (uint32_t)m_VertexBindingDescriptions.size();
    vertexInputCreateInfo.pVertexBindingDescriptions = m_VertexBindingDescriptions.data();
    vertexInputCreateInfo.vertexAttributeDescriptionCount = (uint32_t)m_VertexAttributeDescriptions.size();
    vertexInputCreateInfo.pVertexAttributeDescriptions = m_VertexAttributeDescriptions.data();

    // Describe input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Describe viewport and scissor based on the pipeline target
    VkExtent2D targetExtent = GetPipelineTargetExtent(m_Desc);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)targetExtent.width;
    viewport.height = (float)targetExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = targetExtent.width;
    scissor.extent.height = targetExtent.height;

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
    rasterizationCreateInfo.cullMode = ToVulkanCullMode(renderState.Cull);
    rasterizationCreateInfo.frontFace = renderState.FrontFace == WindingOrder::Clockwise
        ? VK_FRONT_FACE_CLOCKWISE
        : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;

    // Describe multisampling. A pipeline must rasterize at the sample count of the attachments it
    // draws into, which for a surface is the multisampled swapchain target and for a framebuffer is
    // whatever sample count that framebuffer was created with.
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.rasterizationSamples = GetPipelineTargetSampleCount(m_Desc);
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    // Describing color blending
    // Note: all paramaters except blendEnable and colorWriteMask are irrelevant here
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    ApplyBlendMode(renderState.Blend, colorBlendAttachmentState);
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // Note: all attachments must have the same values unless a device feature is enabled
    const std::vector<VkFormat> targetColorFormats = GetPipelineTargetColorFormats(m_Desc);
    const std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates(
        targetColorFormats.size(), colorBlendAttachmentState);

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = (uint32_t)colorBlendAttachmentStates.size();
    colorBlendCreateInfo.pAttachments = colorBlendAttachmentStates.data();
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    // Viewport and scissor are dynamic so one pipeline serves every window size.
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = (renderState.Depth == DepthMode::Test || renderState.Depth == DepthMode::TestWrite) ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = (renderState.Depth == DepthMode::Write || renderState.Depth == DepthMode::TestWrite) ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    if (!m_Layout.Create(*m_VulkanAPI, m_Desc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)) {
        return;
    }

    VkPipelineRenderingCreateInfo renderingCreateInfo = {};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingCreateInfo.colorAttachmentCount = (uint32_t)targetColorFormats.size();
    renderingCreateInfo.pColorAttachmentFormats = targetColorFormats.data();
    renderingCreateInfo.depthAttachmentFormat = GetPipelineTargetDepthFormat(m_Desc);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = &renderingCreateInfo;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = m_Layout.GetHandle();
    pipelineCreateInfo.renderPass = VK_NULL_HANDLE;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_VulkanAPI->GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        AE_ERROR("failed to create graphics pipeline");
        m_Pipeline = VK_NULL_HANDLE;
        return;
    } else {
        AE_INFO("created graphics pipeline");
    }
}

static uint32_t VertexComponents(ShaderDataType InType) {
    switch (InType) {
        case ShaderDataType::Float:  return 1;
        case ShaderDataType::Float2: return 2;
        case ShaderDataType::Float3: return 3;
        case ShaderDataType::Float4: return 4;
        default:
            AE_ERROR("unsupported vertex attribute type in pipeline vertex layout");
            exit(1);
    }
}

static VkFormat VertexFormat(ShaderDataType InType) {
    static constexpr VkFormat s_Formats[] = { VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT };
    return s_Formats[VertexComponents(InType) - 1];
}

void VulkanPipeline::CreateVertexDescriptions() {
    m_VertexAttributeDescriptions.clear();
    m_VertexBindingDescriptions.clear();

    uint32_t location = 0;
    const auto addBinding = [&](const Array<ShaderDataType>& InLayout, uint32_t InBinding, VkVertexInputRate InRate) {
        uint32_t offset = 0;
        for (ShaderDataType type : InLayout) {
            VkVertexInputAttributeDescription attribute = {};
            attribute.binding = InBinding;
            attribute.location = location++;
            attribute.format = VertexFormat(type);
            attribute.offset = offset;
            m_VertexAttributeDescriptions.push_back(attribute);
            offset += VertexComponents(type) * (uint32_t)sizeof(float);
        }
        VkVertexInputBindingDescription binding = {};
        binding.binding = InBinding;
        binding.stride = offset;
        binding.inputRate = InRate;
        m_VertexBindingDescriptions.push_back(binding);
    };

    addBinding(m_Desc.VertexLayout, 0, VK_VERTEX_INPUT_RATE_VERTEX);
    if (!m_Desc.InstanceLayout.IsEmpty()) {
        addBinding(m_Desc.InstanceLayout, 1, VK_VERTEX_INPUT_RATE_INSTANCE);
    }
}

void VulkanPipeline::Destroy() {
    m_Layout.Retire(m_Pipeline);
    m_Pipeline = VK_NULL_HANDLE;
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