#pragma once

#include "vkw/device/device.hpp"

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

        const VkHandle<VkDescriptorSetLayout>& layout() const { return _descriptorSetLayout; }

    private:
        DevicePtr _device;

        VkHandle<VkDescriptorSetLayout> _descriptorSetLayout;
    };
}