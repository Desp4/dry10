#include "desclayout.hpp"
#include "vkw/device/device.hpp"

namespace dry::vkw {

descriptor_layout::descriptor_layout(std::span<const VkDescriptorSetLayoutBinding> bindings) {
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptor_set_layout_info.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(device_main::device(), &descriptor_set_layout_info, NULL_ALLOC, &_descriptor_set_layout);
}

descriptor_layout::~descriptor_layout() {
    vkDestroyDescriptorSetLayout(device_main::device(), _descriptor_set_layout, NULL_ALLOC);
}

}