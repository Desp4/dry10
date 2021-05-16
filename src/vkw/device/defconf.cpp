#include "defconf.hpp"

namespace dry::vkw {

bool check_device_extension_support(VkPhysicalDevice device, std::span<const char* const> extensions) {
    u32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> supported_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, supported_extensions.data());

    // or could use a set or something
    bool extensions_present = true;
    for (auto p = extensions.begin(); p != extensions.end() && extensions_present; ++p) {
        const std::string_view extension = *p;
        for (const auto& supportedExtension : supported_extensions) {
            if (extensions_present = supportedExtension.extensionName == extension) {
                break;
            }
        }
    }
    return extensions_present;
}

bool check_device_feature_support(VkPhysicalDevice device, const VkPhysicalDeviceFeatures& features) {
    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(device, &supported_features);
    const VkBool32* in_iterator = reinterpret_cast<const VkBool32*>(&features);
    const VkBool32* supported_iterator = reinterpret_cast<const VkBool32*>(&supported_features);
    constexpr u32_t limit = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

    // all fields in features are 4byte bools, aligned to 4
    bool features_present = true;
    for (auto i = 0u; i < limit && features_present; ++i) {
        features_present = !(*(in_iterator + i)) || !(*(in_iterator + i) ^ *(supported_iterator + i));
    }
    return features_present;
}

bool check_device_queue_support(VkPhysicalDevice device, VkQueueFlags queue_flags) {
    u32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, families.data());

    for (auto p = families.begin(); p != families.end() && queue_flags; ++p) {
        queue_flags &= ~(queue_flags & p->queueFlags);
    }
    return !queue_flags;
}

bool check_swap_format_support(VkPhysicalDevice device, VkSurfaceKHR surface, VkFormat format, VkColorSpaceKHR color_space) {
    u32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> supported_formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, supported_formats.data());

    for (const VkSurfaceFormatKHR supported_format : supported_formats) {
        if (supported_format.colorSpace == color_space && supported_format.format == format) {
            return true;
        }
    }
    return false;
}

bool check_swap_present_mode_support(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR present_mode) {
    u32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> supported_modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, supported_modes.data());

    for (const auto supported_mode : supported_modes) {
        if (supported_mode == present_mode) {
            return true;
        }
    }
    return false;
}

// get index functions are mostly copies of https://github.com/charles-lunarg/vk-bootstrap/blob/master/src/VkBootstrap.cpp

u32_t get_separate_transfer_index(std::span<const VkQueueFamilyProperties> families) {
    u32_t fallback = (std::numeric_limits<u32_t>::max)();
    for (auto i = 0u; i < families.size(); ++i) {
        const auto flags = families[i].queueFlags;
        if ((flags & VK_QUEUE_TRANSFER_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT)) {
            if (flags & VK_QUEUE_COMPUTE_BIT) {
                fallback = i;
            } else {
                return i;
            }
        }
    }
    return fallback;
}

u32_t get_separate_graphics_index(std::span<const VkQueueFamilyProperties> families) {
    u32_t fallback = (std::numeric_limits<u32_t>::max)();
    for (auto i = 0u; i < families.size(); ++i) {
        const auto flags = families[i].queueFlags;
        if ((flags & VK_QUEUE_GRAPHICS_BIT) && !(flags & VK_QUEUE_TRANSFER_BIT)) {
            if (flags & VK_QUEUE_COMPUTE_BIT) {
                fallback = i;
            } else {
                return i;
            }
        }
    }
    return fallback;
}

u32_t get_separate_compute_index(std::span<const VkQueueFamilyProperties> families) {
    for (auto i = 0u; i < families.size(); ++i) {
        const auto flags = families[i].queueFlags;
        if ((flags & VK_QUEUE_COMPUTE_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT) && !(flags & VK_QUEUE_TRANSFER_BIT)) {
            return i;
        }
    }
    return (std::numeric_limits<u32_t>::max)();
}

u32_t get_present_index(std::span<const VkQueueFamilyProperties> families, VkPhysicalDevice device, VkSurfaceKHR surface) {
    for (auto i = 0u; i < families.size(); ++i) {
        VkBool32 supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
        if (supported == VK_TRUE) {
            return i;
        }
    }
    return (std::numeric_limits<u32_t>::max)();
}

u32_t get_any_index(std::span<const VkQueueFamilyProperties> families, VkQueueFlags flags) {
    for (auto i = 0u; i < families.size(); ++i) {
        if (families[i].queueFlags & flags) {
            return i;
        }
    }
    return (std::numeric_limits<u32_t>::max)();
}

}
