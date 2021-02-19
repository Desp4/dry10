#pragma once

#include <span>
#include <vector>

#include "vkw/vkw.hpp"

namespace dry::vkw {

class device_main {
public:
    struct queue_info {
        uint32_t queue_family_index;
        uint32_t queue_count;
        std::vector<float> priorities;
    };

    static void create(VkPhysicalDevice phys_device, std::span<const queue_info> queue_infos,
                       std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features);
    static void destroy();

    static VkSurfaceCapabilitiesKHR surface_capabilities(VkSurfaceKHR surface);
    static VkPhysicalDeviceMemoryProperties memory_properties();
    // returns UINT32_MAX on failure
    static uint32_t find_memory_type_index(uint32_t type_filter, VkMemoryPropertyFlags properties);
    static void wait_on_device();

    static const VkDevice& device(){
        return _device;
    }

private:
    device_main() = default;

    inline static VkDevice _device = VK_NULL_HANDLE;
    inline static VkPhysicalDevice _phys_device = VK_NULL_HANDLE;

    inline static bool _destroyed = false;
};

}