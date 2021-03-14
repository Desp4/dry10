#pragma once

#include <vector>
#include <span>

#include "vkw/vkw.hpp"

namespace dry::vkw {

class descriptor_pool : public movable<descriptor_pool> {
public:
    using movable<descriptor_pool>::operator=;

    descriptor_pool() = default;
    descriptor_pool(descriptor_pool&&) = default;
    descriptor_pool(std::span<const VkDescriptorPoolSize> pool_sizes, uint32_t capacity);
    ~descriptor_pool();

    void create_sets(VkDescriptorSet* beg, VkDescriptorSetLayout layout, uint32_t count) const;

private:
    vk_handle<VkDescriptorPool> _pool;
};

}