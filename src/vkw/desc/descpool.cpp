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

descriptor_sets descriptor_pool::create_sets(VkDescriptorSetLayout layout, uint32_t count) const {
    descriptor_sets sets(count);
    const std::vector<VkDescriptorSetLayout> layouts(count, layout);

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = _pool;
    set_info.descriptorSetCount = count;
    set_info.pSetLayouts = layouts.data();
    vkAllocateDescriptorSets(device_main::device(), &set_info, sets.data());
    return sets;
}

void descriptor_pool::update_descriptor_set(VkDescriptorSet set, std::span<VkWriteDescriptorSet> descriptor_writes) const {
    for (auto& write : descriptor_writes) {
        write.dstSet = set; // TODO : kinda dumb modifying the writes here, move it to descriptor?
    }
    vkUpdateDescriptorSets(device_main::device(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
}

}