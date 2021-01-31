#include "descpool.hpp"

namespace vkw
{
    DescriptorPool::DescriptorPool(const Device* device, std::span<const VkDescriptorPoolSize> poolSizes, uint32_t capacity) :
        _device(device)
    {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = capacity;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        vkCreateDescriptorPool(_device->device(), &poolInfo, NULL_ALLOC, &_pool);
    }

    DescriptorPool::~DescriptorPool()
    {
        if (_device) vkDestroyDescriptorPool(_device->device(), _pool, NULL_ALLOC);
    }

    DescriptorSets DescriptorPool::createSets(VkDescriptorSetLayout layout, uint32_t count) const
    {
        DescriptorSets sets(count);
        std::vector<VkDescriptorSetLayout> layouts(count, layout);

        VkDescriptorSetAllocateInfo setInfo{};
        setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setInfo.descriptorPool = _pool;
        setInfo.descriptorSetCount = count;
        setInfo.pSetLayouts = layouts.data();
        vkAllocateDescriptorSets(_device->device(), &setInfo, sets.data());
        return sets;
    }

    void DescriptorPool::updateDescriptorSet(VkDescriptorSet set, std::span<VkWriteDescriptorSet> descriptorWrites) const
    {
        for (auto& write : descriptorWrites)
            write.dstSet = set; // FUTURE : kinda dumb modifying the writes here idk, TODO : move it to descriptor?
        vkUpdateDescriptorSets(_device->device(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}