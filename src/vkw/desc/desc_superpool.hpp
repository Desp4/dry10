#pragma once

#ifndef DRY_VK_DESC_SUPERPOOL_H
#define DRY_VK_DESC_SUPERPOOL_H

#include "descpool.hpp"

namespace dry::vkw {

class vk_descriptor_superpool {
public:
    vk_descriptor_superpool() = default;
    vk_descriptor_superpool(const vk_device& device, std::span<const VkDescriptorPoolSize> sizes, u32_t capacity, VkDescriptorSetLayout layout);

    VkDescriptorSet get_descriptor_set();
    void return_descriptor_set(VkDescriptorSet desc_set);

private:
    const vk_device* _device = nullptr;
    std::vector<vkw::vk_descriptor_pool> _desc_pools;
    std::vector<VkDescriptorSet> _available_sets;

    std::vector<VkDescriptorPoolSize> _sizes;
    VkDescriptorSetLayout _layout;
    u32_t _capacity = 0;
};

}

#endif