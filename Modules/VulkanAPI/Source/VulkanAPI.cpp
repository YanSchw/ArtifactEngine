#include "VulkanAPI.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <functional>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Core/Log.h"
#include "Core/EngineConfig.h"
#include "Platform/Platform.h"
#include "Window.h"

#include <cstdlib>
#include <filesystem>
#include "Rendering/Vertex.h"
#include "Rendering/ShaderData.h"

#include "Helpers.h"
#include "VulkanVertexBuffer.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanImage.h"
#include "VulkanTexture.h"
#include "VulkanSampler.h"
#include "VulkanFrameBuffer.h"

#if defined(__APPLE__)
#include <vulkan/vulkan_macos.h>
#endif

const bool ENABLE_DEBUGGING = false;

const char* DEBUG_LAYER = "VK_LAYER_LUNARG_standard_validation";

// Debug callback
VkBool32 debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData) {
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        AE_ERROR("ERROR: [{0}] Code {1} : {2}", pLayerPrefix, msgCode, pMsg);
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        AE_WARN("WARNING: [{0}] Code {1} : {2}", pLayerPrefix, msgCode, pMsg);
    }

    //exit(1);

    return VK_FALSE;
}


static VkInstance instance;
static VkPhysicalDevice physicalDevice;
static VkDevice device;
static VkDebugReportCallbackEXT callback;
static VkQueue graphicsQueue;
static VkQueue presentQueue;
static VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

static VkDescriptorPool descriptorPool;

struct VulkanSwapchainData {
    Surface* Target = nullptr;
    VkSurfaceKHR WindowSurface = VK_NULL_HANDLE;
    VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
    VkExtent2D Extent = { 0, 0 };
    std::vector<VkImage> Images;
    std::vector<VkImageView> ImageViews;
    VkSemaphore ImageAvailable = VK_NULL_HANDLE;
    VkSemaphore RenderingFinished = VK_NULL_HANDLE;
    uint32_t ImageIndex = 0;
    bool Acquired = false;
    // Multisampled color target rendered into, then resolved to the swapchain image each frame.
    // Null when MSAA is disabled (swapChainSampleCount == VK_SAMPLE_COUNT_1_BIT).
    VkImage MsaaImage = VK_NULL_HANDLE;
    VkDeviceMemory MsaaMemory = VK_NULL_HANDLE;
    VkImageView MsaaView = VK_NULL_HANDLE;
};
static std::vector<VulkanSwapchainData*> swapchains;

// Shared across all swapchains: pipelines are built once against this format, and viewport /
// scissor are dynamic state, so surface-targeting pipelines work for every window.
VkExtent2D swapChainExtent = {1280, 720};
VkFormat swapChainFormat = VK_FORMAT_UNDEFINED;
// Multisample count for every surface (swapchain) render target and the pipelines that draw to a
// surface; the result is resolved to the single-sample swapchain image. 1 disables MSAA entirely.
VkSampleCountFlagBits swapChainSampleCount = VK_SAMPLE_COUNT_1_BIT;

static VkCommandPool commandPool;
static VkCommandBuffer graphicsCommandBuffer = VK_NULL_HANDLE;
static VkFence frameFence = VK_NULL_HANDLE;

static uint32_t graphicsQueueFamily;
static uint32_t presentQueueFamily;

// Find device memory that is supported by the requirements (typeBits) and meets the desired properties
VkBool32 GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex) {
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

uint32_t VulkanAPI::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // We can either choose any format
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }

    // Or go with the standard format - if available
    for (const auto& availableSurfaceFormat : availableFormats) {
        if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
            return availableSurfaceFormat;
        }
    }

    // Or fall back to the first available one
    return availableFormats[0];
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, Surface* target) {
    if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
        VkExtent2D extent = {};

        extent.width = std::min(std::max(target->GetWidth(), surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
        extent.height = std::min(std::max(target->GetHeight(), surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);

        return extent;
    }
    else {
        return surfaceCapabilities.currentExtent;
    }
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> presentModes);

static VulkanSwapchainData* FindSwapchainData(const Surface* InTarget) {
    for (VulkanSwapchainData* data : swapchains) {
        if (data->Target == InTarget) {
            return data;
        }
    }
    return nullptr;
}

// (Re)creates the multisampled color target sized to the swapchain extent, releasing any previous
// one first. A no-op that leaves the handles null when MSAA is disabled.
static void CreateMsaaTarget(VulkanSwapchainData& data) {
    if (data.MsaaView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, data.MsaaView, nullptr);
        data.MsaaView = VK_NULL_HANDLE;
    }
    if (data.MsaaImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, data.MsaaImage, nullptr);
        data.MsaaImage = VK_NULL_HANDLE;
    }
    if (data.MsaaMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, data.MsaaMemory, nullptr);
        data.MsaaMemory = VK_NULL_HANDLE;
    }

    if (swapChainSampleCount == VK_SAMPLE_COUNT_1_BIT) {
        return;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { data.Extent.width, data.Extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = swapChainFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    imageInfo.samples = swapChainSampleCount;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageInfo, nullptr, &data.MsaaImage) != VK_SUCCESS) {
        AE_ERROR("failed to create MSAA color target");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, data.MsaaImage, &memRequirements);

    // The multisampled target is never sampled or read back, so prefer memoryless storage when the
    // driver offers it (free on tile-based GPUs such as Apple's), falling back to device-local.
    uint32_t memoryTypeIndex = 0;
    if (!GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &memoryTypeIndex) &&
        !GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex)) {
        AE_ERROR("no suitable memory type for MSAA color target");
        exit(1);
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    if (vkAllocateMemory(device, &allocInfo, nullptr, &data.MsaaMemory) != VK_SUCCESS) {
        AE_ERROR("failed to allocate MSAA color target memory");
        exit(1);
    }
    vkBindImageMemory(device, data.MsaaImage, data.MsaaMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = data.MsaaImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapChainFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &data.MsaaView) != VK_SUCCESS) {
        AE_ERROR("failed to create MSAA color target view");
        exit(1);
    }
}

static void CreateSwapchainForData(VulkanSwapchainData& data) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, data.WindowSurface, &surfaceCapabilities) != VK_SUCCESS) {
        AE_ERROR("failed to acquire presentation surface capabilities");
        exit(1);
    }

    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, data.WindowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
        AE_ERROR("failed to get number of supported surface formats");
        exit(1);
    }
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, data.WindowSurface, &formatCount, surfaceFormats.data()) != VK_SUCCESS) {
        AE_ERROR("failed to get supported surface formats");
        exit(1);
    }

    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, data.WindowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
        AE_ERROR("failed to get number of supported presentation modes");
        exit(1);
    }
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, data.WindowSurface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
        AE_ERROR("failed to get supported presentation modes");
        exit(1);
    }

    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(surfaceFormats);
    if (swapChainFormat == VK_FORMAT_UNDEFINED) {
        swapChainFormat = surfaceFormat.format;
    } else if (surfaceFormat.format != swapChainFormat) {
        AE_WARN("swapchain format mismatch across windows ({0} vs {1})", (int)surfaceFormat.format, (int)swapChainFormat);
    }

    data.Extent = ChooseSwapExtent(surfaceCapabilities, data.Target);
    if (!swapchains.empty() && swapchains[0] == &data) {
        swapChainExtent = data.Extent;
    }

    VkSurfaceTransformFlagBitsKHR surfaceTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        surfaceTransform = surfaceCapabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = data.WindowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = data.Extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = surfaceTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = ChoosePresentMode(presentModes);
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = data.SwapChain;

    VkSwapchainKHR newSwapChain;
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapChain) != VK_SUCCESS) {
        AE_ERROR("failed to create swap chain");
        exit(1);
    }

    for (VkImageView imageView : data.ImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    data.ImageViews.clear();
    if (data.SwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, data.SwapChain, nullptr);
    }
    data.SwapChain = newSwapChain;

    uint32_t actualImageCount = 0;
    if (vkGetSwapchainImagesKHR(device, data.SwapChain, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0) {
        AE_ERROR("failed to acquire number of swap chain images");
        exit(1);
    }
    data.Images.resize(actualImageCount);
    if (vkGetSwapchainImagesKHR(device, data.SwapChain, &actualImageCount, data.Images.data()) != VK_SUCCESS) {
        AE_ERROR("failed to acquire swap chain images");
        exit(1);
    }

    data.ImageViews.resize(data.Images.size());
    for (size_t i = 0; i < data.Images.size(); i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = data.Images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &data.ImageViews[i]) != VK_SUCCESS) {
            AE_ERROR("failed to create image view for swap chain image #{0}", i);
            exit(1);
        }
    }

    CreateMsaaTarget(data);
}

static void CreateSwapchainResources(VulkanSwapchainData& data) {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &data.ImageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &data.RenderingFinished) != VK_SUCCESS) {
        AE_ERROR("failed to create semaphores");
        exit(1);
    }
    CreateSwapchainForData(data);
}

static VulkanSwapchainData* GetOrCreateSwapchainData(Surface* InTarget) {
    if (VulkanSwapchainData* existing = FindSwapchainData(InTarget)) {
        return existing;
    }

    Window* window = InTarget->As<Window>();
    AE_ASSERT(window, "surface render targets must be windows");

    VulkanSwapchainData* data = new VulkanSwapchainData();
    data->Target = InTarget;
    if (glfwCreateWindowSurface(instance, window->GetNativeWindow(), NULL, &data->WindowSurface) != VK_SUCCESS) {
        AE_ERROR("failed to create window surface!");
        exit(1);
    }
    swapchains.push_back(data);
    CreateSwapchainResources(*data);
    return data;
}

static void DestroySwapchainData(VulkanSwapchainData& data) {
    if (data.MsaaView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, data.MsaaView, nullptr);
        data.MsaaView = VK_NULL_HANDLE;
    }
    if (data.MsaaImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, data.MsaaImage, nullptr);
        data.MsaaImage = VK_NULL_HANDLE;
    }
    if (data.MsaaMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, data.MsaaMemory, nullptr);
        data.MsaaMemory = VK_NULL_HANDLE;
    }
    for (VkImageView imageView : data.ImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    data.ImageViews.clear();
    if (data.SwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, data.SwapChain, nullptr);
    }
    if (data.ImageAvailable != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, data.ImageAvailable, nullptr);
    }
    if (data.RenderingFinished != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, data.RenderingFinished, nullptr);
    }
    if (data.WindowSurface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, data.WindowSurface, nullptr);
    }
}

static void RecreateSwapchain(VulkanSwapchainData& data) {
    vkDeviceWaitIdle(device);
    CreateSwapchainForData(data);
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> presentModes) {
    for (const auto& presentMode : presentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    // If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanAPI::Initialize() {
    VulkanAPI::CreateInstance();
    VulkanAPI::CreateDebugCallback();
    VulkanAPI::CreateWindowSurface();
    VulkanAPI::FindPhysicalDevice();
    VulkanAPI::CheckSwapChainSupport();
    VulkanAPI::FindQueueFamilies();
    VulkanAPI::CreateLogicalDevice();
    VulkanAPI::CreateCommandPool();
    VulkanAPI::CreateDescriptorPool();
    CreateSwapchainResources(*swapchains[0]);
    VulkanAPI::CreateCommandBuffers();
}

void VulkanAPI::CleanUp(bool fullClean) {
    vkDeviceWaitIdle(device);

    if (fullClean) {
        vkFreeCommandBuffers(device, commandPool, 1, &graphicsCommandBuffer);
        vkDestroyFence(device, frameFence, nullptr);
        vkDestroyCommandPool(device, commandPool, nullptr);

        VulkanPipeline::DestroyAll();
        VulkanShader::DestroyAll();

        VulkanImageView::DestroyAll();
        VulkanImage::DestroyAll();

        VulkanSampler::DestroyAll();
        VulkanTexture::DestroyAll();

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        VulkanUniformBuffer::DestroyAll();
        VulkanStorageBuffer::DestroyAll();

        // Buffers must be destroyed after no command buffers are referring to them anymore
        VulkanVertexBuffer::DestroyAll();

        for (VulkanSwapchainData* data : swapchains) {
            DestroySwapchainData(*data);
            delete data;
        }
        swapchains.clear();

        vkDestroyDevice(device, nullptr);

        if (ENABLE_DEBUGGING) {
            PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
            DestroyDebugReportCallback(instance, callback, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }
}

void VulkanAPI::DestroySurfaceResources(Surface* InSurface) {
    VulkanSwapchainData* data = FindSwapchainData(InSurface);
    if (!data) {
        return;
    }
    vkDeviceWaitIdle(device);
    DestroySwapchainData(*data);
    swapchains.erase(std::find(swapchains.begin(), swapchains.end(), data));
    delete data;
}

// In a packaged macOS .app the MoltenVK driver and its ICD manifest are bundled inside the app
// (see Package/MacOS.py). The Vulkan loader does not search app bundles, so point it at the
// bundled manifest before the first ICD enumeration. Only set what the user hasn't overridden.
static void PointLoaderAtBundledDrivers() {
#if defined(__APPLE__)
    if (!EngineConfig::IsPackagedBuild()) {
        return;
    }

    const std::filesystem::path manifest =
        std::filesystem::path(Platform::GetResourceDirectory()) / "vulkan" / "icd.d" / "MoltenVK_icd.json";

    if (!std::filesystem::exists(manifest)) {
        AE_WARN("Bundled MoltenVK ICD manifest not found at {0}", manifest.string());
        return;
    }

    // VK_DRIVER_FILES is the modern variable; VK_ICD_FILENAMES is the legacy fallback. Set both so
    // we work regardless of loader version. overwrite=0 lets an explicit user override win.
    setenv("VK_DRIVER_FILES", manifest.string().c_str(), 0);
    setenv("VK_ICD_FILENAMES", manifest.string().c_str(), 0);
#endif
}

void VulkanAPI::CreateInstance() {
    PointLoaderAtBundledDrivers();

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanClear";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ClearScreenEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // Get instance extensions required by GLFW to draw to window
    unsigned int glfwExtensionCount;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions;
    for (size_t i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }

#if defined(__APPLE__)
    extensions.push_back("VK_KHR_portability_enumeration");
#endif

    if (ENABLE_DEBUGGING) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    // Check for extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    if (extensionCount == 0) {
        AE_ERROR("no extensions supported!");
        exit(1);
    }

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    AE_INFO("supported extensions:");

    for (const auto& extension : availableExtensions) {
        AE_INFO("- {0}", extension.extensionName);
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
#if defined(__APPLE__)
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    if (ENABLE_DEBUGGING) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &DEBUG_LAYER;
    }

    // Initialize Vulkan instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        AE_ERROR("failed to create instance!");
        exit(1);
    }
    else {
        AE_INFO("created vulkan instance");
    }
}

void VulkanAPI::CreateWindowSurface() {
    VulkanSwapchainData* data = new VulkanSwapchainData();
    data->Target = Window::GetInstance();
    if (glfwCreateWindowSurface(instance, Window::GetGLFWwindow(), NULL, &data->WindowSurface) != VK_SUCCESS) {
        AE_ERROR("failed to create window surface!");
        exit(1);
    }
    swapchains.push_back(data);

    AE_INFO("created window surface");
}

void VulkanAPI::FindPhysicalDevice() {
    // Try to find 1 Vulkan supported device
    // Note: perhaps refactor to loop through devices and find first one that supports all required features and extensions
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
        AE_ERROR("failed to get number of physical devices");
        exit(1);
    }

    deviceCount = 1;
    VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevice);
    if (res != VK_SUCCESS && res != VK_INCOMPLETE) {
        AE_ERROR("enumerating physical devices failed!");
        exit(1);
    }

    if (deviceCount == 0) {
        AE_ERROR("no physical devices that support vulkan!");
        exit(1);
    }

    AE_INFO("physical device with vulkan support found");

    // Check device features
    // Note: will apiVersion >= appInfo.apiVersion? Probably yes, but spec is unclear.
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    const VkSampleCountFlags colorSampleCounts = deviceProperties.limits.framebufferColorSampleCounts;
    if (colorSampleCounts & VK_SAMPLE_COUNT_4_BIT) {
        swapChainSampleCount = VK_SAMPLE_COUNT_4_BIT;
    } else if (colorSampleCounts & VK_SAMPLE_COUNT_2_BIT) {
        swapChainSampleCount = VK_SAMPLE_COUNT_2_BIT;
    } else {
        swapChainSampleCount = VK_SAMPLE_COUNT_1_BIT;
    }
    AE_INFO("surface MSAA sample count: {0}", (int)swapChainSampleCount);

    uint32_t supportedVersion[] = {
        VK_VERSION_MAJOR(deviceProperties.apiVersion),
        VK_VERSION_MINOR(deviceProperties.apiVersion),
        VK_VERSION_PATCH(deviceProperties.apiVersion)
    };

    AE_INFO("physical device supports version {0}.{1}.{2}", supportedVersion[0], supportedVersion[1], supportedVersion[2]);
}

void VulkanAPI::CheckSwapChainSupport() {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    if (extensionCount == 0) {
        AE_ERROR("physical device doesn't support any extensions");
        exit(1);
    }

    std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, deviceExtensions.data());

    for (const auto& extension : deviceExtensions) {
        if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            AE_INFO("physical device supports swap chains");
            return;
        }
    }

    AE_ERROR("physical device doesn't support swap chains");
    exit(1);
}

void VulkanAPI::FindQueueFamilies() {
    // Check queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    if (queueFamilyCount == 0) {
        AE_INFO("physical device has no queue families!");
        exit(1);
    }

    // Find queue family with graphics support
    // Note: is a transfer queue necessary to copy vertices to the gpu or can a graphics queue handle that?
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    AE_INFO("physical device has {0} queue families", queueFamilyCount);

    bool foundGraphicsQueueFamily = false;
    bool foundPresentQueueFamily = false;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, swapchains[0]->WindowSurface, &presentSupport);

        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily = i;
            foundGraphicsQueueFamily = true;

            if (presentSupport) {
                presentQueueFamily = i;
                foundPresentQueueFamily = true;
                break;
            }
        }

        if (!foundPresentQueueFamily && presentSupport) {
            presentQueueFamily = i;
            foundPresentQueueFamily = true;
        }
    }

    if (foundGraphicsQueueFamily) {
        AE_INFO("queue family #{0} supports graphics", graphicsQueueFamily);

        if (foundPresentQueueFamily) {
            AE_INFO("queue family #{0} supports presentation", presentQueueFamily);
        }
        else {
            AE_ERROR("could not find a valid queue family with present support");
            exit(1);
        }
    }
    else {
        AE_ERROR("could not find a valid queue family with graphics support");
        exit(1);
    }
}

void VulkanAPI::CreateLogicalDevice() {
    // Greate one graphics queue and optionally a separate presentation queue
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo[2] = {};

    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamily;
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriority;

    queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[1].queueFamilyIndex = presentQueueFamily;
    queueCreateInfo[1].queueCount = 1;
    queueCreateInfo[1].pQueuePriorities = &queuePriority;

    // Create logical device from physical device
    // Note: there are separate instance and device extensions!
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

    if (graphicsQueueFamily == presentQueueFamily) {
        deviceCreateInfo.queueCreateInfoCount = 1;
    }
    else {
        deviceCreateInfo.queueCreateInfoCount = 2;
    }

    // Necessary for shader (for some reason)
    VkPhysicalDeviceFeatures enabledFeatures = {};
#ifndef __APPLE__
    enabledFeatures.shaderClipDistance = VK_TRUE;
    enabledFeatures.shaderCullDistance = VK_TRUE;
#endif
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };
    deviceCreateInfo.enabledExtensionCount = 2;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
    deviceCreateInfo.pNext = &dynamicRenderingFeatures;

    if (ENABLE_DEBUGGING) {
        deviceCreateInfo.enabledLayerCount = 1;
        deviceCreateInfo.ppEnabledLayerNames = &DEBUG_LAYER;
    }

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        AE_ERROR("failed to create logical device");
        exit(1);
    }

    AE_INFO("created logical device");

    // Get graphics and presentation queues (which may be the same)
    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);

    AE_INFO("acquired graphics and presentation queues");

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
}

void VulkanAPI::CreateDebugCallback() {
    if (ENABLE_DEBUGGING) {
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

        PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

        if (CreateDebugReportCallback(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
            AE_ERROR("failed to create debug callback");
            exit(1);
        }
        else {
            AE_INFO("created debug callback");
        }
    }
    else {
        AE_INFO("skipped creating debug callback");
    }
}

void VulkanAPI::CreateCommandPool() {
    // Create graphics command pool
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCreateInfo.queueFamilyIndex = graphicsQueueFamily;

    if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        AE_ERROR("failed to create command queue for graphics queue family");
        exit(1);
    }
    else {
        AE_INFO("created command pool for graphics queue family");
    }
}

void VulkanAPI::CreateDescriptorPool() {
    // This describes how many descriptor sets we'll create from this pool for each type
    VkDescriptorPoolSize typeCounts[2] = {};
    typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    typeCounts[0].descriptorCount = 1000; // total UBO bindings across all sets
    typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    typeCounts[1].descriptorCount = 1000; // total sampler bindings across all sets

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // A rebuilt pipeline's old set is retired (freed a frame later, see VulkanPipeline::Destroy),
    // so it coexists with the replacement set
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.poolSizeCount = 2;
    createInfo.pPoolSizes = typeCounts;
    createInfo.maxSets = 1000; // number of descriptor sets

    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        AE_ERROR("failed to create descriptor pool");
        exit(1);
    }
    else {
        AE_INFO("created descriptor pool");
    }
}

void VulkanAPI::CreateCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &graphicsCommandBuffer) != VK_SUCCESS) {
        AE_ERROR("failed to allocate graphics command buffers");
        exit(1);
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(device, &fenceInfo, nullptr, &frameFence) != VK_SUCCESS) {
        AE_ERROR("failed to create frame fence");
        exit(1);
    }

    AE_INFO("allocated graphics command buffers");
}

// True only when the surface's actual pixel extent differs from the swapchain built for it.
// This is the reliable signal for "needs recreation" — unlike the window-resize callback flag,
// which on macOS/MoltenVK can latch on essentially every frame even when nothing changed.
static bool HasSurfaceExtentChanged(const VulkanSwapchainData& data) {
    VkSurfaceCapabilitiesKHR caps;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, data.WindowSurface, &caps) != VK_SUCCESS) {
        return false;
    }
    // 0xFFFFFFFF => the extent is chosen by the swapchain, not the surface; nothing to compare.
    if (caps.currentExtent.width == 0xFFFFFFFF) {
        return false;
    }
    // Zero extent (e.g. minimized) can't produce a valid swapchain; don't spin recreating it.
    if (caps.currentExtent.width == 0 || caps.currentExtent.height == 0) {
        return false;
    }
    return caps.currentExtent.width != data.Extent.width
        || caps.currentExtent.height != data.Extent.height;
}

void VulkanAPI::Draw() {
    std::vector<VulkanSwapchainData*> targets;
    for (const RenderCommand& cmd : m_RenderQueue.commands) {
        if (cmd.Type != RenderCommandType::BeginRenderPass) {
            continue;
        }
        const auto& data = std::get<CmdBeginRenderPass>(cmd.Data);
        Object* targetObject = data.Target.Get();
        Surface* surface = targetObject ? targetObject->As<Surface>() : nullptr;
        if (!surface) {
            continue;
        }
        VulkanSwapchainData* swapchain = GetOrCreateSwapchainData(surface);
        if (std::find(targets.begin(), targets.end(), swapchain) == targets.end()) {
            targets.push_back(swapchain);
        }
    }

    if (targets.empty()) {
        m_RenderQueue.Clear();
        return;
    }

    // Acquiring re-signals ImageAvailable, so the previous frame must have consumed it first.
    // The fence is only reset once past the point where this frame can still bail out.
    vkWaitForFences(device, 1, &frameFence, VK_TRUE, UINT64_MAX);

    VulkanPipeline::FlushRetired();

    for (VulkanSwapchainData* swapchain : targets) {
        swapchain->Acquired = false;
        VkResult res = vkAcquireNextImageKHR(device, swapchain->SwapChain, UINT64_MAX, swapchain->ImageAvailable, VK_NULL_HANDLE, &swapchain->ImageIndex);

        // Only a genuinely out-of-date swapchain must be recreated before we can draw.
        // VK_SUBOPTIMAL_KHR still presents fine; recreating on it never converges on MoltenVK.
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            AE_TRACE("swapchain out of date on acquire");
            RecreateSwapchain(*swapchain);
            m_RenderQueue.Clear();
            return;
        }
        else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
            AE_ERROR("failed to acquire image");
            exit(1);
        }
        swapchain->Acquired = true;
    }

    vkResetFences(device, 1, &frameFence);

    vkResetCommandBuffer(graphicsCommandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(graphicsCommandBuffer, &beginInfo);

    VkImageSubresourceRange subResourceRange = {};
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    for (VulkanSwapchainData* swapchain : targets) {
        VkImageMemoryBarrier presentToDrawBarrier = {};
        presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentToDrawBarrier.srcAccessMask = 0;
        presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentToDrawBarrier.image = swapchain->Images[swapchain->ImageIndex];
        presentToDrawBarrier.subresourceRange = subResourceRange;

        vkCmdPipelineBarrier(graphicsCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToDrawBarrier);

        if (swapchain->MsaaImage != VK_NULL_HANDLE) {
            VkImageMemoryBarrier msaaBarrier = {};
            msaaBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            msaaBarrier.srcAccessMask = 0;
            msaaBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            msaaBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            msaaBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            msaaBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            msaaBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            msaaBarrier.image = swapchain->MsaaImage;
            msaaBarrier.subresourceRange = subResourceRange;

            vkCmdPipelineBarrier(graphicsCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &msaaBarrier);
        }
    }

    RecordCommandBuffer(m_RenderQueue, graphicsCommandBuffer);

    if (vkEndCommandBuffer(graphicsCommandBuffer) != VK_SUCCESS) {
        AE_ERROR("failed to record command buffer");
        exit(1);
    }
    m_RenderQueue.Clear();

    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<VkSemaphore> signalSemaphores;
    for (VulkanSwapchainData* swapchain : targets) {
        waitSemaphores.push_back(swapchain->ImageAvailable);
        waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        signalSemaphores.push_back(swapchain->RenderingFinished);
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsCommandBuffer;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameFence) != VK_SUCCESS) {
        AE_ERROR("failed to submit draw command buffer");
        exit(1);
    }

    std::vector<VkSwapchainKHR> presentSwapchains;
    std::vector<uint32_t> presentImageIndices;
    std::vector<VkResult> presentResults(targets.size(), VK_SUCCESS);
    for (VulkanSwapchainData* swapchain : targets) {
        presentSwapchains.push_back(swapchain->SwapChain);
        presentImageIndices.push_back(swapchain->ImageIndex);
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = (uint32_t)signalSemaphores.size();
    presentInfo.pWaitSemaphores = signalSemaphores.data();
    presentInfo.swapchainCount = (uint32_t)presentSwapchains.size();
    presentInfo.pSwapchains = presentSwapchains.data();
    presentInfo.pImageIndices = presentImageIndices.data();
    presentInfo.pResults = presentResults.data();

    const VkResult presentRes = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (presentRes != VK_SUCCESS && presentRes != VK_SUBOPTIMAL_KHR && presentRes != VK_ERROR_OUT_OF_DATE_KHR) {
        AE_ERROR("failed to submit present command buffer");
        exit(1);
    }

    for (size_t i = 0; i < targets.size(); i++) {
        if (presentResults[i] == VK_ERROR_OUT_OF_DATE_KHR || HasSurfaceExtentChanged(*targets[i])) {
            RecreateSwapchain(*targets[i]);
        }
    }
}

void VulkanAPI::RecordCommandBuffer(RenderCommandQueue& InQueue, VkCommandBuffer InCmdBuffer) {
    bool hasRenderPassBegun = false;
    VulkanPipeline* currentPipeline = nullptr;

    for (const RenderCommand& cmd : InQueue.commands) {
        switch (cmd.Type) {
            case RenderCommandType::BeginRenderPass: {
                const auto& data = std::get<CmdBeginRenderPass>(cmd.Data);
                if (hasRenderPassBegun) {
                    vkCmdEndRendering(InCmdBuffer);
                }

                if (VulkanFrameBuffer* framebuffer = data.Target ? data.Target->As<VulkanFrameBuffer>() : nullptr) {
                    auto colorAttachmentInfo = framebuffer->GetColorAttachmentInfo();
                    auto depthAttachmentInfo = framebuffer->GetDepthAttachmentInfo();

                    VkRenderingInfo renderingInfo{};
                    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                    renderingInfo.renderArea = { 0, 0, framebuffer->GetDesc().Width, framebuffer->GetDesc().Height };
                    renderingInfo.layerCount = 1;
                    renderingInfo.colorAttachmentCount = framebuffer->GetColorAttachmentCount();
                    renderingInfo.pColorAttachments = colorAttachmentInfo.data();
                    renderingInfo.pDepthAttachment = &depthAttachmentInfo;

                    for (size_t i = 0; i < framebuffer->GetColorAttachmentCount(); i++) {
                        VulkanHelpers::TransitionImage(InCmdBuffer,
                            framebuffer->GetDesc().ColorAttachments[i]->GetDesc().ImagePtr->As<VulkanImage>()->GetVkImage(),
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            VK_IMAGE_ASPECT_COLOR_BIT);
                    }

                    if (depthAttachmentInfo.imageView) {
                        VulkanHelpers::TransitionImage(InCmdBuffer,
                            framebuffer->GetDesc().DepthAttachment->GetDesc().ImagePtr->As<VulkanImage>()->GetVkImage(),
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_ASPECT_DEPTH_BIT);
                    }

                    vkCmdBeginRendering(InCmdBuffer, &renderingInfo);

                    const VkExtent2D extent = { framebuffer->GetDesc().Width, framebuffer->GetDesc().Height };
                    VkViewport viewport = { 0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f };
                    VkRect2D scissor = { { 0, 0 }, extent };
                    vkCmdSetViewport(InCmdBuffer, 0, 1, &viewport);
                    vkCmdSetScissor(InCmdBuffer, 0, 1, &scissor);
                } else if (Surface* surface = data.Target ? data.Target->As<Surface>() : nullptr) {
                    VulkanSwapchainData* swapchain = FindSwapchainData(surface);
                    AE_ASSERT(swapchain && swapchain->Acquired, "surface target has no acquired swapchain image");

                    VkRenderingAttachmentInfo swapchainColorAttachment{};
                    swapchainColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    swapchainColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    swapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    swapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    swapchainColorAttachment.clearValue = { 0.1f, 0.1f, 0.1f, 1.0f };
                    if (swapchain->MsaaView != VK_NULL_HANDLE) {
                        // Draw into the multisampled target and resolve it down to the swapchain
                        // image; the resolve covers the whole area, so the swapchain image needs
                        // no clear and the multisampled contents need not be stored.
                        swapchainColorAttachment.imageView = swapchain->MsaaView;
                        swapchainColorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
                        swapchainColorAttachment.resolveImageView = swapchain->ImageViews[swapchain->ImageIndex];
                        swapchainColorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        swapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    } else {
                        swapchainColorAttachment.imageView = swapchain->ImageViews[swapchain->ImageIndex];
                    }

                    VkRenderingInfo renderingInfo{};
                    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                    renderingInfo.renderArea = { 0, 0, swapchain->Extent.width, swapchain->Extent.height };
                    renderingInfo.layerCount = 1;
                    renderingInfo.colorAttachmentCount = 1;
                    renderingInfo.pColorAttachments = &swapchainColorAttachment;
                    renderingInfo.pDepthAttachment = nullptr;

                    vkCmdBeginRendering(InCmdBuffer, &renderingInfo);

                    VkViewport viewport = { 0.0f, 0.0f, (float)swapchain->Extent.width, (float)swapchain->Extent.height, 0.0f, 1.0f };
                    VkRect2D scissor = { { 0, 0 }, swapchain->Extent };
                    vkCmdSetViewport(InCmdBuffer, 0, 1, &viewport);
                    vkCmdSetScissor(InCmdBuffer, 0, 1, &scissor);
                } else {
                    AE_ASSERT(false, "unsupported render target type");
                }

                hasRenderPassBegun = true;
                break;
            }

            case RenderCommandType::BindPipeline: {
                const auto& data = std::get<CmdBindPipeline>(cmd.Data);

                if (!data.Pipeline)
                    continue;

                VulkanPipeline* pipeline = data.Pipeline->As<VulkanPipeline>();
                currentPipeline = pipeline;

                vkCmdBindDescriptorSets(InCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->m_PipelineLayout, 0, 1, &pipeline->m_DescriptorSet, 0, nullptr);
                vkCmdBindPipeline(InCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->m_Pipeline);
                break;
            }

            case RenderCommandType::BindVertexBuffer: {
                const auto& data = std::get<CmdBindVertexBuffer>(cmd.Data);

                if (!data.Buffer)
                    continue;

                VulkanVertexBuffer* vkBuffer = data.Buffer->As<VulkanVertexBuffer>();

                VkBuffer vertexBuffers[] = { vkBuffer->m_VertexBuffer };
                VkDeviceSize offsets[] = { 0 };

                vkCmdBindVertexBuffers(InCmdBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(InCmdBuffer, vkBuffer->m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32 );
                break;
            }

            case RenderCommandType::DrawIndexed: {
                const auto& data = std::get<CmdDrawIndexed>(cmd.Data);
                vkCmdDrawIndexed(InCmdBuffer, data.IndexCount, 1, data.FirstIndex, data.VertexOffset, 0);
                break;
            }

            case RenderCommandType::SetShaderData: {
                const auto& data = std::get<CmdSetShaderData>(cmd.Data);

                if (!data.Data)
                    continue;

                AE_ASSERT(currentPipeline, "A Pipeline has to be bound for ShaderData to be uploaded!");
                AE_ASSERT(data.Data->Size() <= VulkanPipeline::MAX_SHADER_DATA_SIZE);

                vkCmdPushConstants(
                    InCmdBuffer,
                    currentPipeline->m_PipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT |
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    static_cast<uint32_t>(data.Data->Size()),
                    data.Data->Data());

                break;
            }
        }
    }

    if (hasRenderPassBegun) {
        vkCmdEndRendering(InCmdBuffer);
    }
}

VkDevice VulkanAPI::GetDevice() const {
    return device;
}

VkPhysicalDevice VulkanAPI::GetPhysicalDevice() const {
    return physicalDevice;
}

void VulkanAPI::WaitIdle() {
    vkDeviceWaitIdle(device);
}

VkCommandPool VulkanAPI::GetCommandPool() const {
    return commandPool;
}

VkDescriptorPool VulkanAPI::GetDescriptorPool() const {
    return descriptorPool;
}

VkQueue VulkanAPI::GetGraphicsQueue() const {
    return graphicsQueue;
}

RenderCommandQueue& VulkanAPI::GetRenderQueue() {
    return m_RenderQueue;
}

SharedObjectPtr<VertexBuffer> VulkanAPI::CreateVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) {
    return new VulkanVertexBuffer(InVertexData, InVertexByteSize, InIndices, *this);
}

SharedObjectPtr<VertexBuffer> VulkanAPI::CreateDynamicVertexBuffer() {
    return new VulkanVertexBuffer(*this);
}

SharedObjectPtr<Shader> VulkanAPI::CreateShader(const String& InShaderSource) {
    return new VulkanShader(InShaderSource, *this);
}

SharedObjectPtr<Pipeline> VulkanAPI::CreatePipeline(const PipelineDesc& InPipelineDesc) {
    return new VulkanPipeline(InPipelineDesc, *this);
}

void VulkanAPI::InvalidateAllPipelines() {
    VulkanPipeline::InvalidateAll();
}

SharedObjectPtr<UniformBuffer> VulkanAPI::CreateUniformBuffer(uint32_t InBinding, size_t InSize) {
    return new VulkanUniformBuffer(InBinding, InSize, *this);
}

SharedObjectPtr<StorageBuffer> VulkanAPI::CreateStorageBuffer(uint32_t InBinding, size_t InSize) {
    return new VulkanStorageBuffer(InBinding, InSize, *this);
}

SharedObjectPtr<Image> VulkanAPI::CreateImage(const ImageDesc& InImageDesc) {
    return new VulkanImage(InImageDesc, *this);
}

SharedObjectPtr<ImageView> VulkanAPI::CreateImageView(const ImageViewDesc& InImageViewDesc) {
    return new VulkanImageView(InImageViewDesc, *this);
}

SharedObjectPtr<Texture> VulkanAPI::CreateTexture(const String& InFilePath, const TextureDesc& InTextureDesc) {
    return new VulkanTexture(InFilePath, InTextureDesc, *this);
}

SharedObjectPtr<Texture> VulkanAPI::CreateTexture(byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels, const TextureDesc& InTextureDesc) {
    return new VulkanTexture(InPixels, InWidth, InHeight, InChannels, InTextureDesc, *this);
}

SharedObjectPtr<Sampler> VulkanAPI::CreateSampler(const SamplerDesc& InSamplerDesc) {
    return new VulkanSampler(InSamplerDesc, *this);
}

SharedObjectPtr<FrameBuffer> VulkanAPI::CreateFrameBuffer(const FrameBufferDesc& InFrameBufferDesc) {
    return new VulkanFrameBuffer(InFrameBufferDesc, *this);
}