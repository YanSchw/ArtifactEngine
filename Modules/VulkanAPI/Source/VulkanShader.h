#pragma once
#include "Rendering/Shader.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>
#include "VulkanShader.gen.h"

class VulkanShader : public Shader {
public:
    ARTIFACT_CLASS();

    VulkanShader(const CompiledShader& InCompiledShader, VulkanAPI& InVulkanAPI);
    virtual ~VulkanShader();
    virtual ShaderType GetShaderType() const override;
    virtual void Reload(const CompiledShader& InCompiledShader) override;

    static void DestroyAll();

private:
    void CreateModules(const CompiledShader& InCompiledShader);
    void DestroyModules();

private:
    VkShaderModule m_VertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_FragmentShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_ComputeShaderModule = VK_NULL_HANDLE;

    VulkanAPI* m_VulkanAPI = nullptr;

    friend class VulkanAPI;
    friend class VulkanPipeline;
};
