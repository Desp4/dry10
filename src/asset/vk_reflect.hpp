#pragma once

#ifndef DRY_VK_REFLECT_H
#define DRY_VK_REFLECT_H

#include <vulkan/vulkan.h>

#include "asset_src.hpp"
#include "dbg/log.hpp"

namespace dry::asset {

struct vk_shader_data {
    template<typename T>
    struct descriptor_info {
        u32_t binding_ind;
        T info;
    };

    VkVertexInputBindingDescription vertex_binding;
    std::vector<VkVertexInputAttributeDescription> vertex_descriptors;

    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;

    std::vector<descriptor_info<VkDescriptorBufferInfo>> buffer_infos;
    std::vector<descriptor_info<VkDescriptorImageInfo>> comb_sampler_infos; // TODO : info not used, why keep it
};

vk_shader_data shader_vk_info(const shader_source& shader, std::vector<VkDescriptorSetLayoutBinding> exclude);

constexpr VkShaderStageFlagBits shader_vk_stage(shader_stage stage) {
    switch (stage) {
    case shader_stage::vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
    case shader_stage::fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
    default: dbg::panic();
    }
}

constexpr VkFormat texture_vk_format(const texture_source& tex) {
    switch (tex.channels) {
    case 1:  return VK_FORMAT_R8_SRGB;
    case 2:  return VK_FORMAT_R8G8_SRGB;
    case 3:  return VK_FORMAT_R8G8B8_SRGB;
    case 4:  return VK_FORMAT_R8G8B8A8_SRGB;
    default: return VK_FORMAT_UNDEFINED;
    }
}

}

#endif DRY_VK_REFLECT_H
