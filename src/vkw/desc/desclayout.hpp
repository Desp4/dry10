#pragma once

#ifndef DRY_VK_DESCLAYOUT_H
#define DRY_VK_DESCLAYOUT_H

#include "vkw/vkw.hpp"

namespace dry::vkw {

class vk_descriptor_layout {
public:
    vk_descriptor_layout(std::span<const VkDescriptorSetLayoutBinding> bindings);

    vk_descriptor_layout() = default;
    vk_descriptor_layout(vk_descriptor_layout&& oth) { *this = std::move(oth); }
    ~vk_descriptor_layout();

    VkDescriptorSetLayout handle() const { return _descriptor_set_layout; }

    vk_descriptor_layout& operator=(vk_descriptor_layout&&);

private:
    VkDescriptorSetLayout _descriptor_set_layout = VK_NULL_HANDLE;
};

}

#endif
