#pragma once

#include <vector>
#include <span>

#include "vkw/vkw.hpp"

namespace dry::vkw {

using descriptor_sets = std::vector<VkDescriptorSet>;

class descriptor_pool : public movable<descriptor_pool> {
public:
    using movable<descriptor_pool>::operator=;

    descriptor_pool() = default;
    descriptor_pool(descriptor_pool&&) = default;
    descriptor_pool(std::span<const VkDescriptorPoolSize> pool_sizes, uint32_t capacity);
    ~descriptor_pool();

    descriptor_sets create_sets(VkDescriptorSetLayout layout, uint32_t count) const;
    void update_descriptor_set(VkDescriptorSet set, std::span<VkWriteDescriptorSet> descriptor_writes) const;

private:
    vk_handle<VkDescriptorPool> _pool;
};

}