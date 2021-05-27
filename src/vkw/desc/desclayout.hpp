#pragma once

#ifndef DRY_VK_DESCLAYOUT_H
#define DRY_VK_DESCLAYOUT_H

#include "vkw/device/device.hpp"

namespace dry::vkw {

class vk_descriptor_layout {
public:
    vk_descriptor_layout(const vk_device& device, std::span<const VkDescriptorSetLayoutBinding> bindings);

    vk_descriptor_layout() = default;
    vk_descriptor_layout(vk_descriptor_layout&& oth) { *this = std::move(oth); }
    ~vk_descriptor_layout();

    VkDescriptorSetLayout handle() const { return _descriptor_set_layout; }

    vk_descriptor_layout& operator=(vk_descriptor_layout&&);

private:
    const vk_device* _device = nullptr;
    VkDescriptorSetLayout _descriptor_set_layout = VK_NULL_HANDLE;
};

}

#endif
