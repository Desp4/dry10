#pragma once

#include "descsets.hpp"

namespace vkw
{
    class DescriptorPool : public Movable<DescriptorPool>
    {
    public:
        using Movable<DescriptorPool>::operator=;

        DescriptorPool() = default;
        DescriptorPool(DescriptorPool&&) = default;
        DescriptorPool(const Device* device, std::span<const VkDescriptorPoolSize> poolSizes, uint32_t capacity);
        ~DescriptorPool();

        DescriptorSets createSets(VkDescriptorSetLayout layout, uint32_t count) const;
        void updateDescriptorSet(VkDescriptorSet set, std::span<VkWriteDescriptorSet> descriptorWrites) const;

    private:
        DevicePtr _device;

        VkHandle<VkDescriptorPool> _pool;
    };
}