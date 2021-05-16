#pragma once

#ifndef DRY_VK_DEVICE_H
#define DRY_VK_DEVICE_H

#include "vkw/vkw.hpp"

namespace dry::vkw {

struct queue_info {
    u32_t queue_family_index;
    u32_t queue_count;
    std::vector<float> priorities;
};

class vk_device {
public:
    vk_device(
        VkPhysicalDevice phys_device, std::span<const queue_info> queue_infos,
        std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features
    );

    vk_device() = default;
    vk_device(vk_device&& oth) { *this = std::move(oth); }
    ~vk_device();

    VkSurfaceCapabilitiesKHR surface_capabilities(VkSurfaceKHR surface) const;
    VkPhysicalDeviceMemoryProperties memory_properties() const;
    // returns UINT32_MAX on failure
    u32_t find_memory_type_index(u32_t type_filter, VkMemoryPropertyFlags properties) const;
    void wait_on_device() const;

    VkDevice handle()  const { return _device; }

    vk_device& operator=(vk_device&&);

private:
    VkDevice _device = VK_NULL_HANDLE;
    VkPhysicalDevice _phys_device = VK_NULL_HANDLE;
};

}

#endif
