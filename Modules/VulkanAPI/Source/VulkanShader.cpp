#include "VulkanShader.h"
#include "Platform/Platform.h"
#include "Platform/Subprocess.h"
#include "Platform/FileIO.h"

static Array<VulkanShader*> s_Shaders;

VkShaderModule CreateShaderModule(const ByteString& InShaderBytes, VulkanAPI& InVulkanAPI) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = InShaderBytes.GetSizeInBytes();
    createInfo.pCode = (uint32_t*)InShaderBytes.GetData();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(InVulkanAPI.GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        AE_ERROR("failed to create shader module");
        exit(1);
    }

    return shaderModule;
}

VulkanShader::VulkanShader(const String& InShaderSource, VulkanAPI& InVulkanAPI) {
    if (InShaderSource.empty()) {
        AE_ERROR("cannot create shader from empty source");
        return;
    }

    s_Shaders.Add(this);
    m_VulkanAPI = &InVulkanAPI;
    
    try {
        Map<String, String> shaderCode = PreProcess(InShaderSource);
        Platform::TemporaryDirectory tempDir;
        for (const auto& [shaderType, code] : shaderCode) {
            // Write shader source to temporary file
            String tempFilePath = std::format("{0}/shader.{1}.glsl", tempDir.Path, shaderType);
            if (!FileIO::WriteStringToFile(tempFilePath, code)) {
                AE_ERROR("failed to write shader source to temporary file");
                return;
            }

            // Compile shader to SPIR-V using glslangValidator
            auto result = Subprocess::Run(std::format("glslangValidator -V {0} -o {1}/shader.{2}.spv", tempFilePath, tempDir.Path, shaderType));
            if (result.ExitCode != 0) {
                AE_ERROR("failed to compile shader:\n{0}", result.StdOut);
                return;
            }

            // Read compiled SPIR-V bytes and create shader module
            auto shaderBytes = FileIO::ReadFileToBytes(std::format("{0}/shader.{1}.spv", tempDir.Path, shaderType));
            if (!shaderBytes) {
                AE_ERROR("failed to read compiled shader bytes");
                return;
            }
            
            if (shaderType == "vert") {
                m_VertexShaderModule = CreateShaderModule(*shaderBytes, InVulkanAPI);
            }
            else if (shaderType == "frag") {
                m_FragmentShaderModule = CreateShaderModule(*shaderBytes, InVulkanAPI);
            }
            else if (shaderType == "comp") {
                m_ComputeShaderModule = CreateShaderModule(*shaderBytes, InVulkanAPI);
            } else {
                AE_ERROR("unknown shader type: {0}", shaderType);
                return;
            }

        }
    } catch (const std::exception& e) {
        AE_ERROR("failed to compile shader: {0}", e.what());
        return;
    }
    
}

VulkanShader::~VulkanShader() {
    s_Shaders.Remove(this);

    if (m_VertexShaderModule) {
        vkDestroyShaderModule(m_VulkanAPI->GetDevice(), m_VertexShaderModule, nullptr);
    }
    if (m_FragmentShaderModule) {
        vkDestroyShaderModule(m_VulkanAPI->GetDevice(), m_FragmentShaderModule, nullptr);
    }
    if (m_ComputeShaderModule) {
        vkDestroyShaderModule(m_VulkanAPI->GetDevice(), m_ComputeShaderModule, nullptr);
    }
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
