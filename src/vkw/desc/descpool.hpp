#pragma once

#ifndef DRY_VK_DESCPOOL_H
#define DRY_VK_DESCPOOL_H

#include "vkw/device/device.hpp"

namespace dry::vkw {

class vk_descriptor_pool {
public:
    vk_descriptor_pool(const vk_device& device, std::span<const VkDescriptorPoolSize> pool_sizes, u32_t capacity);

    vk_descriptor_pool() = default;
    vk_descriptor_pool(vk_descriptor_pool&& oth) { *this = std::move(oth); }
    ~vk_descriptor_pool();

    void create_sets(std::span<VkDescriptorSet> sets, VkDescriptorSetLayout layout) const;

    vk_descriptor_pool& operator=(vk_descriptor_pool&&);

private:
    const vk_device* _device = nullptr;
    VkDescriptorPool _pool = VK_NULL_HANDLE;
};

}

#endif
