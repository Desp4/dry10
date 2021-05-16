#pragma once

#include "vkw/desc/descpool.hpp"

namespace dry::gr {

class descriptor_pool_pool {
public:
    descriptor_pool_pool() = default;
    descriptor_pool_pool(std::span<const VkDescriptorPoolSize> sizes, uint32_t capacity, VkDescriptorSetLayout layout);

    VkDescriptorSet get_descriptor_set();
    void return_descriptor_set(VkDescriptorSet desc_set);

    void update_descriptor_set(VkDescriptorSet desc_set, std::span<VkWriteDescriptorSet> desc_writes) const;

private:
    std::vector<VkDescriptorPoolSize> _sizes;
    uint32_t _capacity;
    VkDescriptorSetLayout _layout;

    std::vector<vkw::vk_descriptor_pool> _pools;
    std::vector<VkDescriptorSet> _available_sets;
};

}