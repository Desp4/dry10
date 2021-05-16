#include "desclayout.hpp"

#include "vkw/device/g_device.hpp"

namespace dry::vkw {

vk_descriptor_layout::vk_descriptor_layout(std::span<const VkDescriptorSetLayoutBinding> bindings) {
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.bindingCount = static_cast<u32_t>(bindings.size());
    descriptor_set_layout_info.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(g_device->handle(), &descriptor_set_layout_info, null_alloc, &_descriptor_set_layout);
}

vk_descriptor_layout::~vk_descriptor_layout() {
    vkDestroyDescriptorSetLayout(g_device->handle(), _descriptor_set_layout, null_alloc);
}

vk_descriptor_layout& vk_descriptor_layout::operator=(vk_descriptor_layout&& oth) {
    // destroy
    vkDestroyDescriptorSetLayout(g_device->handle(), _descriptor_set_layout, null_alloc);
    // move
    _descriptor_set_layout = oth._descriptor_set_layout;
    // null
    oth._descriptor_set_layout = VK_NULL_HANDLE;
    return *this;
}

}
