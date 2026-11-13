
# Link to Artifact Module
cmake_policy(SET CMP0079 NEW)
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_include_directories(VulkanAPI PUBLIC "${AE_VULKAN_INCLUDE_DIR}")
    target_link_libraries(VulkanAPI
        PUBLIC "${AE_MOLTENVK_FRAMEWORK}"
        "-framework UIKit"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
        "-framework IOSurface"
        "-framework CoreGraphics"
    )
elseif (APPLE)
    find_package(Vulkan REQUIRED)
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
    find_package(Vulkan REQUIRED)
    target_link_libraries(VulkanAPI
        PUBLIC Vulkan::Vulkan
    )
endif()
