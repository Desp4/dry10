#include "desc_pool_pool.hpp"

#include "vkw/device/device.hpp"

namespace dry::gr {

descriptor_pool_pool::descriptor_pool_pool(std::span<const VkDescriptorPoolSize> sizes, uint32_t capacity, VkDescriptorSetLayout layout) :
    _sizes(sizes.begin(), sizes.end()),
    _capacity{ capacity },
    _layout{ layout },
    _available_sets(capacity)
{
    _pools.push_back(vkw::descriptor_pool{sizes, capacity});
    _pools.back().create_sets(_available_sets.data(), layout, capacity);
}

VkDescriptorSet descriptor_pool_pool::get_descriptor_set() {
    if (_available_sets.size() == 0) {
        _pools.push_back(vkw::descriptor_pool{ _sizes, _capacity });
        _available_sets.resize(_capacity);
        _pools.back().create_sets(_available_sets.data(), _layout, _capacity);
    }

    const auto ret_desc_set = _available_sets.back();
    _available_sets.pop_back();
    return ret_desc_set;
}

void descriptor_pool_pool::return_descriptor_set(VkDescriptorSet desc_set) {
    _available_sets.push_back(desc_set);
}

void descriptor_pool_pool::update_descriptor_set(VkDescriptorSet desc_set, std::span<VkWriteDescriptorSet> desc_writes) const {
    for (auto& write : desc_writes) {
        write.dstSet = desc_set; // TODO : kinda dumb modifying the writes here, move it to descriptor?
    }
    vkUpdateDescriptorSets(vkw::device_main::device(), static_cast<uint32_t>(desc_writes.size()), desc_writes.data(), 0, nullptr);
}

}