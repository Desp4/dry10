#include "descpool.hpp"

#include "vkw/device/g_device.hpp"

namespace dry::vkw {

vk_descriptor_pool::vk_descriptor_pool(std::span<const VkDescriptorPoolSize> pool_sizes, u32_t capacity) {
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = capacity;
    pool_info.poolSizeCount = static_cast<u32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    vkCreateDescriptorPool(g_device->handle(), &pool_info, null_alloc, &_pool);
}

vk_descriptor_pool::~vk_descriptor_pool() {
    vkDestroyDescriptorPool(g_device->handle(), _pool, null_alloc);
}

void vk_descriptor_pool::create_sets(std::span<VkDescriptorSet> sets, VkDescriptorSetLayout layout) const {
    const std::vector<VkDescriptorSetLayout> layouts(sets.size(), layout);

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = _pool;
    set_info.descriptorSetCount = static_cast<u32_t>(sets.size());
    set_info.pSetLayouts = layouts.data();
    vkAllocateDescriptorSets(g_device->handle(), &set_info, sets.data());
}

vk_descriptor_pool& vk_descriptor_pool::operator=(vk_descriptor_pool&& oth) {
    // destroy
    vkDestroyDescriptorPool(g_device->handle(), _pool, null_alloc);
    // move
    _pool = oth._pool;
    // null
    oth._pool = VK_NULL_HANDLE;
    return *this;
}

}
