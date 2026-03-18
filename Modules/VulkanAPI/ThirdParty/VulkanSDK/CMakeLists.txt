
find_package(Vulkan REQUIRED)

# Link to Artifact Module
cmake_policy(SET CMP0079 NEW)
if (APPLE)
    # macOS frameworks
    target_link_libraries(VulkanAPI
        PUBLIC Vulkan::Vulkan
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreFoundation" 
        "-framework CoreVideo"
        "-framework Metal"
        "-framework QuartzCore" 
        "-framework Foundation" 
        "-framework IOSurface" 
        "-framework CoreGraphics"
        "-framework AppKit"
        "/usr/local/lib/libMoltenVK.dylib"
        "/usr/local/lib/libvulkan.dylib"
    )
else()
    target_link_libraries(VulkanAPI
        PUBLIC Vulkan::Vulkan
    )
endif()