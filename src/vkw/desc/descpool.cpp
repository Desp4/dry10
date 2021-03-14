#include "descpool.hpp"
#include "vkw/device/device.hpp"

namespace dry::vkw {

descriptor_pool::descriptor_pool(std::span<const VkDescriptorPoolSize> pool_sizes, uint32_t capacity) {
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = capacity;
    pool_info.poolSizeCount = pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    vkCreateDescriptorPool(device_main::device(), &pool_info, NULL_ALLOC, &_pool);
}

descriptor_pool::~descriptor_pool() {
    vkDestroyDescriptorPool(device_main::device(), _pool, NULL_ALLOC);
}

void descriptor_pool::create_sets(VkDescriptorSet* beg, VkDescriptorSetLayout layout, uint32_t count) const {
    const std::vector<VkDescriptorSetLayout> layouts(count, layout);

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = _pool;
    set_info.descriptorSetCount = count;
    set_info.pSetLayouts = layouts.data();
    vkAllocateDescriptorSets(device_main::device(), &set_info, beg);
}

}