#pragma once

#ifndef DRY_VK_DEVICE_H
#define DRY_VK_DEVICE_H

#include "instance.hpp"

#include <vk_mem_alloc.h>

namespace dry::vkw {

struct queue_info {
    u32_t queue_family_index;
    u32_t queue_count;
    std::vector<float> priorities;
};

class vk_device {
public:
    vk_device(const vk_instance& instance, VkPhysicalDevice phys_device, std::span<const queue_info> queue_infos,
        std::span<const char* const> extensions, const VkPhysicalDeviceFeatures& features
    ) noexcept;

    vk_device() noexcept = default;
    vk_device(vk_device&& oth) noexcept { *this = std::move(oth); }
    ~vk_device();

    VkSurfaceCapabilitiesKHR surface_capabilities(VkSurfaceKHR surface) const;
    const VkPhysicalDeviceMemoryProperties& memory_properties() const { return _mem_properties; }
    VkDeviceSize pad_uniform_size(VkDeviceSize size) const;
    // returns UINT32_MAX on failure
    u32_t find_memory_type_index(u32_t type_filter, VkMemoryPropertyFlags properties) const;
    void wait_on_device() const;

    VkDevice handle() const { return _device; }
    VmaAllocator allocator() const { return _allocator; }

    vk_device& operator=(vk_device&&) noexcept;

private:
    VkDevice _device = VK_NULL_HANDLE;
    VkPhysicalDevice _phys_device = VK_NULL_HANDLE;
    VmaAllocator _allocator = VK_NULL_HANDLE;

    VkPhysicalDeviceMemoryProperties _mem_properties;
    VkPhysicalDeviceProperties _device_properties;
};

}

#endif
