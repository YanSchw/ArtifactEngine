#pragma once

class VulkanAPI {
public:
    static void SetupVulkan();
    
    static void CreateInstance();
    static void CreateDebugCallback();
    static void CreateWindowSurface();
    static void FindPhysicalDevice();
    static void CheckSwapChainSupport();
    static void FindQueueFamilies();
    static void CreateLogicalDevice();
    static void CreateSemaphores();
    static void CreateCommandPool();
    static void CreateVertexBuffer();
    static void CreateUniformBuffer();
    static void CreateSwapChain();
    static void CreateRenderPass();
    static void CreateImageViews();
    static void CreateFramebuffers();
    static void CreateGraphicsPipeline();
    static void CreateDescriptorPool();
    static void CreateDescriptorSet();
    static void CreateCommandBuffers();

    static void UpdateUniformData();

    static void CleanUp(bool fullClean);

    static void OnWindowSizeChanged();
    static void Draw();
};