#pragma once

#include <vector>
#include <span>

#include <vulkan/vulkan.h>

namespace vkw::conf
{
    bool deviceExtensionsSupport(VkPhysicalDevice device, std::span<const char* const> extensions);
    bool deviceFeaturesSupport(VkPhysicalDevice device, const VkPhysicalDeviceFeatures& features);
    bool deviceQueuesSupport(VkPhysicalDevice device, VkQueueFlags queueFlags); // present == graphics

    bool swapFormatSupport(VkPhysicalDevice device, VkSurfaceKHR surface, VkFormat format, VkColorSpaceKHR colorSpace);
    bool swapPresentModeSupport(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR presentMode);

    // find index functions return -1 if not found

    int32_t findSeparateTransferIndex(std::span<const VkQueueFamilyProperties> families);
    int32_t findSeparateGraphicsIndex(std::span<const VkQueueFamilyProperties> families);
    int32_t findSeparateComputeIndex(std::span<const VkQueueFamilyProperties> families);
    int32_t findPresentIndex(std::span<const VkQueueFamilyProperties> families, VkPhysicalDevice device, VkSurfaceKHR surface);
    int32_t findAnyIndex(std::span<const VkQueueFamilyProperties> families, VkQueueFlags flags);
}