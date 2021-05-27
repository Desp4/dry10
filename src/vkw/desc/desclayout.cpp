#include "desclayout.hpp"

namespace dry::vkw {

vk_descriptor_layout::vk_descriptor_layout(const vk_device& device, std::span<const VkDescriptorSetLayoutBinding> bindings) :
    _device{ &device }
{
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.bindingCount = static_cast<u32_t>(bindings.size());
    descriptor_set_layout_info.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(_device->handle(), &descriptor_set_layout_info, null_alloc, &_descriptor_set_layout);
}

vk_descriptor_layout::~vk_descriptor_layout() {
    if (_device != nullptr) {
        vkDestroyDescriptorSetLayout(_device->handle(), _descriptor_set_layout, null_alloc);
    }
}

vk_descriptor_layout& vk_descriptor_layout::operator=(vk_descriptor_layout&& oth) {
    // destroy
    if (_device != nullptr) {
        vkDestroyDescriptorSetLayout(_device->handle(), _descriptor_set_layout, null_alloc);
    }
    // move
    _device = oth._device;
    _descriptor_set_layout = oth._descriptor_set_layout;
    // null
    oth._device = nullptr;
    return *this;
}

}
