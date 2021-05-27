#include "desc_superpool.hpp"

namespace dry::vkw {

vk_descriptor_superpool::vk_descriptor_superpool(const vk_device& device, std::span<const VkDescriptorPoolSize> sizes, u32_t capacity, VkDescriptorSetLayout layout) :
    _device{ &device },
    _available_sets(capacity),
    _sizes{ sizes.begin(), sizes.end() },
    _layout{ layout },
    _capacity{ capacity }
{
    _desc_pools.push_back(vk_descriptor_pool{ *_device, _sizes, _capacity });
    _desc_pools.back().create_sets(_available_sets, layout);
}

VkDescriptorSet vk_descriptor_superpool::get_descriptor_set() {
    if (_available_sets.size() == 0) {
        _desc_pools.push_back(vkw::vk_descriptor_pool{ *_device, _sizes, _capacity });
        _available_sets.resize(_capacity);
        _desc_pools.back().create_sets(_available_sets, _layout);
    }

    const auto ret_desc_set = _available_sets.back();
    _available_sets.pop_back();
    return ret_desc_set;
}

void vk_descriptor_superpool::return_descriptor_set(VkDescriptorSet desc_set) {
    _available_sets.push_back(desc_set);
}

}
