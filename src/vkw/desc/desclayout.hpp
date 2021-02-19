#pragma once

#include <span>

#include "vkw/vkw.hpp"

namespace dry::vkw {

class descriptor_layout : public movable<descriptor_layout> {
public:
    using movable<descriptor_layout>::operator=;

    descriptor_layout() = default;
    descriptor_layout(descriptor_layout&&) = default;
    descriptor_layout(std::span<const VkDescriptorSetLayoutBinding> bindings);
    ~descriptor_layout();

    const VkDescriptorSetLayout& layout() const {
        return _descriptor_set_layout;
    }

private:
    vk_handle<VkDescriptorSetLayout> _descriptor_set_layout;
};

}