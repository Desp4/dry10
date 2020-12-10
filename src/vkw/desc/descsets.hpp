#pragma once

#include <vector>

#include "../device/device.hpp"
// TODO : FUTURE : this thing with an array interface too
namespace vkw
{
    class DescriptorSets
    {
    public:
        DescriptorSets() = default;

        const std::vector<VkDescriptorSet>& sets() const { return _sets; }

    private:
        friend class DescriptorPool;
        DescriptorSets(uint32_t count) : _sets(count) {}

        std::vector<VkDescriptorSet> _sets;
    };
}