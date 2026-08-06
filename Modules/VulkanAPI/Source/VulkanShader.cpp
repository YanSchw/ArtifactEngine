#include "VulkanShader.h"

static Array<VulkanShader*> s_Shaders;

static VkShaderModule CreateShaderModule(const ByteString& InShaderBytes, VulkanAPI& InVulkanAPI) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = InShaderBytes.GetSizeInBytes();
    createInfo.pCode = (uint32_t*)InShaderBytes.GetData();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(InVulkanAPI.GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        AE_ERROR("failed to create shader module");
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

VulkanShader::VulkanShader(const CompiledShader& InCompiledShader, VulkanAPI& InVulkanAPI) {
    s_Shaders.Add(this);
    m_VulkanAPI = &InVulkanAPI;
    CreateModules(InCompiledShader);
}

VulkanShader::~VulkanShader() {
    s_Shaders.Remove(this);
    DestroyModules();
}

void VulkanShader::CreateModules(const CompiledShader& InCompiledShader) {
    m_RenderState = InCompiledShader.RenderState;

    for (const CompiledShaderStage& stage : InCompiledShader.Stages) {
        VkShaderModule shaderModule = CreateShaderModule(*stage.ByteCode, *m_VulkanAPI);
        if (shaderModule == VK_NULL_HANDLE) {
            continue;
        }

        switch (stage.Stage) {
            case ShaderStage::Vertex:   m_VertexShaderModule = shaderModule; break;
            case ShaderStage::Fragment: m_FragmentShaderModule = shaderModule; break;
            case ShaderStage::Compute:  m_ComputeShaderModule = shaderModule; break;
        }
    }
}

void VulkanShader::DestroyModules() {
    if (m_VertexShaderModule) {
        vkDestroyShaderModule(m_VulkanAPI->GetDevice(), m_VertexShaderModule, nullptr);
        m_VertexShaderModule = VK_NULL_HANDLE;
    }
    if (m_FragmentShaderModule) {
        vkDestroyShaderModule(m_VulkanAPI->GetDevice(), m_FragmentShaderModule, nullptr);
        m_FragmentShaderModule = VK_NULL_HANDLE;
    }
    if (m_ComputeShaderModule) {
        vkDestroyShaderModule(m_VulkanAPI->GetDevice(), m_ComputeShaderModule, nullptr);
        m_ComputeShaderModule = VK_NULL_HANDLE;
    }
}

void VulkanShader::Reload(const CompiledShader& InCompiledShader) {
    DestroyModules();
    CreateModules(InCompiledShader);
}

ShaderType VulkanShader::GetShaderType() const {
    if (m_VertexShaderModule && m_FragmentShaderModule) {
        return ShaderType::VertexFragment;
    }
    else if (m_ComputeShaderModule) {
        return ShaderType::Compute;
    }
    else {
        return ShaderType::Unkown;
    }
}

void VulkanShader::DestroyAll() {
    Array<VulkanShader*> shadersToDestroy = s_Shaders;
    for (VulkanShader* shader : shadersToDestroy) {
        delete shader;
    }
    s_Shaders.Clear();
}
