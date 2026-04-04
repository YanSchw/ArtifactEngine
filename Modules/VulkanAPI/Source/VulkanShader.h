#pragma once
#include "Rendering/Shader.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanShader.gen.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>

class VulkanShader : public Shader {
public:
    ARTIFACT_CLASS();

    VulkanShader(const String& InShaderSource, VulkanAPI& InVulkanAPI);
    virtual ~VulkanShader();
    virtual ShaderType GetShaderType() const override;

    static void DestroyAll();

private:
    VkShaderModule m_VertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_FragmentShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_ComputeShaderModule = VK_NULL_HANDLE;

    VulkanAPI* m_VulkanAPI = nullptr;

    friend class VulkanAPI;
    friend class VulkanPipeline;
};