#include "descpool.hpp"

namespace dry::vkw {

vk_descriptor_pool::vk_descriptor_pool(const vk_device& device, std::span<const VkDescriptorPoolSize> pool_sizes, u32_t capacity) :
    _device{ &device }
{
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = capacity;
    pool_info.poolSizeCount = static_cast<u32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    vkCreateDescriptorPool(_device->handle(), &pool_info, null_alloc, &_pool);
}

vk_descriptor_pool::~vk_descriptor_pool() {
    if (_device != nullptr) {
        vkDestroyDescriptorPool(_device->handle(), _pool, null_alloc);
    }
}

void vk_descriptor_pool::create_sets(std::span<VkDescriptorSet> sets, VkDescriptorSetLayout layout) const {
    const std::vector<VkDescriptorSetLayout> layouts(sets.size(), layout);

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = _pool;
    set_info.descriptorSetCount = static_cast<u32_t>(sets.size());
    set_info.pSetLayouts = layouts.data();
    vkAllocateDescriptorSets(_device->handle(), &set_info, sets.data());
}

vk_descriptor_pool& vk_descriptor_pool::operator=(vk_descriptor_pool&& oth) {
    // destroy
    if (_device != nullptr) {
        vkDestroyDescriptorPool(_device->handle(), _pool, null_alloc);
    }
    // move
    _device = oth._device;
    _pool = oth._pool;
    // null
    oth._device = nullptr;
    return *this;
}

}
