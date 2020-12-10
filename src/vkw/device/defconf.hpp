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

    std::vector<uint32_t> getQueueFamilyIndices(VkPhysicalDevice device, VkQueueFlags queueFamilies);
}