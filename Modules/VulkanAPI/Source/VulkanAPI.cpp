#include "VulkanAPI.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <functional>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Core/Log.h"
#include "Window.h"
#include "Rendering/Vertex.h"

#include "VulkanVertexBuffer.h"
#include "VulkanShader.h"
#include "VulkanPipeline.h"

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
static VkSurfaceKHR windowSurface;
static VkPhysicalDevice physicalDevice;
static VkDevice device;
static VkDebugReportCallbackEXT callback;
static VkQueue graphicsQueue;
static VkQueue presentQueue;
static VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
static VkSemaphore imageAvailableSemaphore;
static VkSemaphore renderingFinishedSemaphore;

VkVertexInputBindingDescription vertexBindingDescription;
std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

struct {
    glm::mat4 transformationMatrix;
} uniformBufferData;
VkBuffer uniformBuffer;
static VkDeviceMemory uniformBufferMemory;
static VkDescriptorPool descriptorPool;
 
VkExtent2D swapChainExtent = {1280, 720};
VkFormat swapChainFormat;
VkSwapchainKHR oldSwapChain;
VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;
std::vector<VkFramebuffer> swapChainFramebuffers;
 
VkRenderPass renderPass;
 
static VkCommandPool commandPool;
static std::vector<VkCommandBuffer> graphicsCommandBuffers;
 
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

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
    if (surfaceCapabilities.currentExtent.width == -1) {
        VkExtent2D swapChainExtent = {};

        swapChainExtent.width = std::min(std::max(Window::GetInstance()->GetWidth(), surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
        swapChainExtent.height = std::min(std::max(Window::GetInstance()->GetHeight(), surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);
        
        return swapChainExtent;
    }
    else {
        return surfaceCapabilities.currentExtent;
    }
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
    oldSwapChain = VK_NULL_HANDLE;

    VulkanAPI::CreateInstance();
    VulkanAPI::CreateDebugCallback();
    VulkanAPI::CreateWindowSurface();
    VulkanAPI::FindPhysicalDevice();
    VulkanAPI::CheckSwapChainSupport();
    VulkanAPI::FindQueueFamilies();
    VulkanAPI::CreateLogicalDevice();
    VulkanAPI::CreateSemaphores();
    VulkanAPI::CreateCommandPool();
    VulkanAPI::CreateVertexDescriptions();
    VulkanAPI::CreateUniformBuffer();
    VulkanAPI::CreateSwapChain();
    VulkanAPI::CreateRenderPass();
    VulkanAPI::CreateImageViews();
    VulkanAPI::CreateFramebuffers();
    VulkanAPI::CreateDescriptorPool();
    VulkanAPI::CreateCommandBuffers();
}

void VulkanAPI::CleanUp(bool fullClean) {
    vkDeviceWaitIdle(device);

    vkFreeCommandBuffers(device, commandPool, (uint32_t)graphicsCommandBuffers.size(), graphicsCommandBuffers.data());

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    if (fullClean) {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderingFinishedSemaphore, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        VulkanPipeline::DestroyAll();
        VulkanShader::DestroyAll();
        // Clean up uniform buffer related objects
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyBuffer(device, uniformBuffer, nullptr);
        vkFreeMemory(device, uniformBufferMemory, nullptr);

        // Buffers must be destroyed after no command buffers are referring to them anymore
        VulkanVertexBuffer::DestroyAll();

        // Note: implicitly destroys images (in fact, we're not allowed to do that explicitly)
        vkDestroySwapchainKHR(device, swapChain, nullptr);

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, windowSurface, nullptr);

        if (ENABLE_DEBUGGING) {
            PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
            DestroyDebugReportCallback(instance, callback, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }
}

void VulkanAPI::CreateInstance() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanClear";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ClearScreenEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

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
    if (glfwCreateWindowSurface(instance, Window::GetGLFWwindow(), NULL, &windowSurface) != VK_SUCCESS) {
        AE_ERROR("failed to create window surface!");
        exit(1);
    }

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
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport);

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

    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = presentQueueFamily;
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriority;

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

    const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

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

void VulkanAPI::CreateSemaphores() {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &createInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &createInfo, nullptr, &renderingFinishedSemaphore) != VK_SUCCESS) {
        AE_ERROR("failed to create semaphores");
        exit(1);
    }
    else {
        AE_INFO("created semaphores");
    }
}

void VulkanAPI::CreateCommandPool() {
    // Create graphics command pool
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = graphicsQueueFamily;

    if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        AE_ERROR("failed to create command queue for graphics queue family");
        exit(1);
    }
    else {
        AE_INFO("created command pool for graphics queue family");
    }
}

void VulkanAPI::CreateVertexDescriptions() {
    // Binding and attribute descriptions
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(Vertex);
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // vec2 position
    vertexAttributeDescriptions.resize(2);
    vertexAttributeDescriptions[0].binding = 0;
    vertexAttributeDescriptions[0].location = 0;
    vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;

    // vec3 color
    vertexAttributeDescriptions[1].binding = 0;
    vertexAttributeDescriptions[1].location = 1;
    vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[1].offset = sizeof(float) * 3;
}

void VulkanAPI::CreateUniformBuffer() {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(uniformBufferData);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, uniformBuffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);

    vkAllocateMemory(device, &allocInfo, nullptr, &uniformBufferMemory);
    vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);

    RenderingAPI::GetInstance()->UpdateUniformData();
}

void VulkanAPI::UpdateUniformData() {
    static auto timeStart = std::chrono::high_resolution_clock::now(); // Initialize once

    auto timeNow = std::chrono::high_resolution_clock::now();
    long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - timeStart).count();
    float angle = (millis % 4000) / 4000.0f * glm::radians(360.f);

    // Proper identity initialization
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0, 0, 1));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f / 3.0f, -0.5f / 3.0f, 0.0f));

    // View and projection
    auto viewMatrix = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
    auto projMatrix = glm::perspective(glm::radians(70.f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

    uniformBufferData.transformationMatrix = projMatrix * viewMatrix * modelMatrix;

    // Upload
    void* data;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(uniformBufferData), 0, &data);
    memcpy(data, &uniformBufferData, sizeof(uniformBufferData));
    vkUnmapMemory(device, uniformBufferMemory);

}

void VulkanAPI::CreateSwapChain() {
    // Find surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities) != VK_SUCCESS) {
        AE_ERROR("failed to acquire presentation surface capabilities");
        exit(1);
    }

    // Find supported surface formats
    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
        AE_ERROR("failed to get number of supported surface formats");
        exit(1);
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, surfaceFormats.data()) != VK_SUCCESS) {
        AE_ERROR("failed to get supported surface formats");
        exit(1);
    }

    // Find supported present modes
    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
        AE_ERROR("failed to get number of supported presentation modes");
        exit(1);
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
        AE_ERROR("failed to get supported presentation modes");
        exit(1);
    }

    // Determine number of images for swap chain
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    AE_INFO("using {0} images for swap chain", imageCount);

    // Select a surface format
    VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(surfaceFormats);

    // Select swap chain size
    swapChainExtent = ChooseSwapExtent(surfaceCapabilities);

    // Determine transformation to use (preferring no transform)
    VkSurfaceTransformFlagBitsKHR surfaceTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        surfaceTransform = surfaceCapabilities.currentTransform;
    }

    // Choose presentation mode (preferring MAILBOX ~= triple buffering)
    VkPresentModeKHR presentMode = ChoosePresentMode(presentModes);

    // Finally, create the swap chain
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = surfaceTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapChain;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        AE_ERROR("failed to create swap chain");
        exit(1);
    }
    else {
        AE_INFO("created swap chain");
    }

    if (oldSwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
    }
    oldSwapChain = swapChain;

    swapChainFormat = surfaceFormat.format;

    // Store the images used by the swap chain
    // Note: these are the images that swap chain image indices refer to
    // Note: actual number of images may differ from requested number, since it's a lower bound
    uint32_t actualImageCount = 0;
    if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0) {
        AE_ERROR("failed to acquire number of swap chain images");
        exit(1);
    }

    swapChainImages.resize(actualImageCount);

    if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, swapChainImages.data()) != VK_SUCCESS) {
        AE_ERROR("failed to acquire swap chain images");
        exit(1);
    }

    AE_INFO("acquired swap chain images");
}

void VulkanAPI::CreateRenderPass() {
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = swapChainFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Note: hardware will automatically transition attachment to the specified layout
    // Note: index refers to attachment descriptions array
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Note: this is a description of how the attachments of the render pass will be used in this sub pass
    // e.g. if they will be read in shaders and/or drawn to
    VkSubpassDescription subPassDescription = {};
    subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDescription.colorAttachmentCount = 1;
    subPassDescription.pColorAttachments = &colorAttachmentReference;

    // Create the render pass
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &attachmentDescription;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subPassDescription;

    if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
        AE_ERROR("failed to create render pass");
        exit(1);
    }
    else {
        AE_INFO("created render pass");
    }
}

void VulkanAPI::CreateImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    // Create an image view for every image in the swap chain
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            AE_ERROR("failed to create image view for swap chain image #{0}", i);
            exit(1);
        }
    }

    AE_INFO("created image views for swap chain images");
}

void VulkanAPI::CreateFramebuffers() {
    swapChainFramebuffers.resize(swapChainImages.size());

    // Note: Framebuffer is basically a specific choice of attachments for a render pass
    // That means all attachments must have the same dimensions, interesting restriction
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &swapChainImageViews[i];
        createInfo.width = swapChainExtent.width;
        createInfo.height = swapChainExtent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            AE_ERROR("failed to create framebuffer for swap chain image view #{0}", i);
            exit(1);
        }
    }

    AE_INFO("created framebuffers for swap chain image views");
}

void VulkanAPI::CreateDescriptorPool() {
    // This describes how many descriptor sets we'll create from this pool for each type
    VkDescriptorPoolSize typeCount;
    typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    typeCount.descriptorCount = 1;

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &typeCount;
    createInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        AE_ERROR("failed to create descriptor pool");
        exit(1);
    }
    else {
        AE_INFO("created descriptor pool");
    }
}

void VulkanAPI::CreateCommandBuffers() {
    // Allocate graphics command buffers
    graphicsCommandBuffers.resize(swapChainImages.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)swapChainImages.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS) {
        AE_ERROR("failed to allocate graphics command buffers");
        exit(1);
    }
    else {
        AE_INFO("allocated graphics command buffers");
    }
}

void VulkanAPI::UpdateCommandBuffer(size_t i) {
    // Reset the command buffer to allow re-recording
    vkResetCommandBuffer(graphicsCommandBuffers[i], 0);

    // Begin recording the command buffer
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vkBeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo);

    // Prepare subresource range for barriers
    VkImageSubresourceRange subResourceRange = {};
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    // Barrier to transition image layout for drawing
    VkImageMemoryBarrier presentToDrawBarrier = {};
    presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentToDrawBarrier.srcAccessMask = 0;
    presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if (presentQueueFamily != graphicsQueueFamily) {
        presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    } else {
        presentToDrawBarrier.srcQueueFamilyIndex = presentQueueFamily;
        presentToDrawBarrier.dstQueueFamilyIndex = graphicsQueueFamily;
    }

    presentToDrawBarrier.image = swapChainImages[i];
    presentToDrawBarrier.subresourceRange = subResourceRange;

    vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToDrawBarrier);

    // Begin render pass
    VkClearValue clearColor = {
        { 0.1f, 0.1f, 0.1f, 1.0f } // R, G, B, A
    };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent = swapChainExtent;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(graphicsCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Record the render commands using RenderCommandQueue
    VulkanAPI::Get().RecordCommandBuffer(VulkanAPI::Get().GetRenderQueue(), graphicsCommandBuffers[i]);

    // End render pass
    vkCmdEndRenderPass(graphicsCommandBuffers[i]);

    // Barrier if present and graphics queue families differ
    if (presentQueueFamily != graphicsQueueFamily) {
        VkImageMemoryBarrier drawToPresentBarrier = {};
        drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        drawToPresentBarrier.srcQueueFamilyIndex = graphicsQueueFamily;
        drawToPresentBarrier.dstQueueFamilyIndex = presentQueueFamily;
        drawToPresentBarrier.image = swapChainImages[i];
        drawToPresentBarrier.subresourceRange = subResourceRange;

        vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &drawToPresentBarrier);
    }

    // End recording the command buffer
    if (vkEndCommandBuffer(graphicsCommandBuffers[i]) != VK_SUCCESS) {
        AE_ERROR("failed to record command buffer");
        exit(1);
    }

    // Clear the render queue after recording
    VulkanAPI::Get().GetRenderQueue().Clear();
}

void VulkanAPI::OnWindowSizeChanged() {
    //windowResized = false;
    Window::GetInstance()->SetResizedFlag(false);

    // Only recreate objects that are affected by framebuffer size changes
    RenderingAPI::GetInstance()->CleanUp(false);

    VulkanAPI::CreateSwapChain();
    VulkanAPI::CreateRenderPass();
    VulkanAPI::CreateImageViews();
    VulkanAPI::CreateFramebuffers();
    Pipeline::InvalidateAll();
    VulkanAPI::CreateCommandBuffers();
}

void VulkanAPI::Draw() {
    // Acquire image
    uint32_t imageIndex;
    VkResult res = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Unless surface is out of date right now, defer swap chain recreation until end of this frame
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        AE_TRACE("swapchain image fault {0}", (int64_t) res);
        VulkanAPI::OnWindowSizeChanged();
        return;
    }
    else if (res != VK_SUCCESS) {
        AE_ERROR("failed to acquire image");
        exit(1);
    }

    // Update the command buffer for the acquired swap chain image with the current render queue
    VulkanAPI::UpdateCommandBuffer(imageIndex);

    // Wait for image to be available and draw
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

    // This is the stage where the queue should wait on the semaphore
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsCommandBuffers[imageIndex];

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        AE_ERROR("failed to submit draw command buffer");
        exit(1);
    }

    // Present drawn image
    // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;

    res = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || Window::GetInstance()->WasWindowResized()) {
        VulkanAPI::OnWindowSizeChanged();
    }
    else if (res != VK_SUCCESS) {
        AE_ERROR("failed to submit present command buffer");
        exit(1);
    }
}

void VulkanAPI::RecordCommandBuffer(RenderCommandQueue& InQueue, VkCommandBuffer InCmdBuffer) {
    for (const RenderCommand& cmd : InQueue.commands) {
        switch (cmd.Type) {
            case RenderCommandType::BindPipeline: {
                const auto& data = std::get<CmdBindPipeline>(cmd.Data);

                if (!data.Pipeline)
                    continue;

                VulkanPipeline* pipeline = data.Pipeline->As<VulkanPipeline>();

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
        }
    }
}

VkDevice VulkanAPI::GetDevice() const {
    return device;
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

SharedObjectPtr<VertexBuffer> VulkanAPI::CreateVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) {
    Window::GetInstance()->SetResizedFlag(true);
    return new VulkanVertexBuffer(InVertices, InIndices, *this);
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