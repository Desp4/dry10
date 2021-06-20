#pragma once

#ifndef DRY_VK_REFLECT_H
#define DRY_VK_REFLECT_H

#include <vulkan/vulkan.h>

#include "asset_src.hpp"
#include "dbg/log.hpp"

namespace dry::asset {
// TODO : don't keep buffer infos at all, don't take excludes and includes, just dump reflection info
struct vk_shader_data {
    struct vertex_binding_info {
        u32_t stride;
        u32_t location;
        VkFormat format;
    };
    struct layout_binding_info {
        u32_t binding;
        u32_t set;
        u32_t count;
        u32_t stride;
        VkShaderStageFlags stage;
        VkDescriptorType type;
    };

    std::vector<vertex_binding_info> vertex_descriptors;
    std::vector<layout_binding_info> layout_bindings;
};

vk_shader_data shader_vk_info(const shader_source& shader);

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
