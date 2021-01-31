#include "desclayout.hpp"

namespace vkw
{
    DescriptorLayout::DescriptorLayout(const Device* device, std::span<const VkDescriptorSetLayoutBinding> bindings) :
        _device(device)
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(_device->device(), &descriptorSetLayoutInfo, NULL_ALLOC, &_descriptorSetLayout);
    }

    DescriptorLayout::~DescriptorLayout()
    {
        if (_device) vkDestroyDescriptorSetLayout(_device->device(), _descriptorSetLayout, NULL_ALLOC);
    }
}