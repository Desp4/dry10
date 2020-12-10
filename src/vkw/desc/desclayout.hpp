#pragma once

#include "../device/device.hpp"

namespace vkw
{
    class DescriptorLayout : public Movable<DescriptorLayout>
    {
    public:
        using Movable<DescriptorLayout>::operator=;

        DescriptorLayout() = default;
        DescriptorLayout(DescriptorLayout&&) = default;
        DescriptorLayout(const Device* device, std::span<const VkDescriptorSetLayoutBinding> bindings);
        ~DescriptorLayout();

        const VkDescriptorSetLayout& layout() const;

    private:
        DevicePtr _device;

        VkHandle<VkDescriptorSetLayout> _descriptorSetLayout;
    };
}