#pragma once

#ifndef DRY_GR_VK_INITERS_H
#define DRY_GR_VK_INITERS_H

#include "vkw/vkw.hpp"

namespace dry {

constexpr VkWriteDescriptorSet desc_write_from_binding(VkDescriptorSetLayoutBinding binding) {
    VkWriteDescriptorSet desc_write{};

    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstBinding = binding.binding;
    desc_write.descriptorType = binding.descriptorType;
    desc_write.descriptorCount = binding.descriptorCount;
    return desc_write;
}

constexpr VkBufferUsageFlags descriptor_type_to_buffer_usage(VkDescriptorType type) {
    switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    default: LOG_ERR("Can't initialize a buffer from this descriptor"); dbg::panic(); // TODO : fmt type into log err
    }
}

}

#endif