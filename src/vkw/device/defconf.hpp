#pragma once

#include <vector>
#include <span>

#include <vulkan/vulkan.h>

namespace dry::vkw::conf {

bool check_device_extension_support(VkPhysicalDevice device, std::span<const char* const> extensions);
bool check_device_feature_support(VkPhysicalDevice device, const VkPhysicalDeviceFeatures& features);
bool check_device_queue_support(VkPhysicalDevice device, VkQueueFlags queue_flags);

bool check_swap_format_support(VkPhysicalDevice device, VkSurfaceKHR surface, VkFormat format, VkColorSpaceKHR color_space);
bool check_swap_present_mode_support(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR present_mode);

// find index functions return -1 if not found

int32_t get_separate_transfer_index(std::span<const VkQueueFamilyProperties> families);
int32_t get_separate_graphics_index(std::span<const VkQueueFamilyProperties> families);
int32_t get_separate_compute_index(std::span<const VkQueueFamilyProperties> families);
int32_t get_present_index(std::span<const VkQueueFamilyProperties> families, VkPhysicalDevice device, VkSurfaceKHR surface);
int32_t get_any_index(std::span<const VkQueueFamilyProperties> families, VkQueueFlags flags);

}