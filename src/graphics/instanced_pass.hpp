#pragma once

#ifndef DRY_GR_UNSHADED_PASS_H
#define DRY_GR_UNSHADED_PASS_H

#include <array>

#include <glm/mat4x4.hpp>

#include "vk_initers.hpp"

#include "vkw/buffer.hpp"
#include "vkw/desc/descpool.hpp"
#include "vkw/desc/desclayout.hpp"

namespace dry {

struct camera_transform {
    glm::mat4 viewproj;
    glm::mat4 proj;
    glm::mat4 view;
};

struct object_transform {
    glm::mat4 model;
};

struct instanced_pass {
    struct alignas(16) instance_input {
        object_transform transform;
        u32_t material;
    };

    // per frame
    std::vector<vkw::vk_buffer> camera_transforms;
    std::vector<vkw::vk_buffer> instance_buffers;
    std::vector<VkDescriptorSet> instance_descriptors;

    vkw::vk_buffer instance_staging_buffer;

    vkw::vk_descriptor_layout instanced_descriptor_layout;
    vkw::vk_descriptor_pool instanced_descriptor_pool;

    static constexpr u32_t combined_instance_buffer_count = 4096 * 8; // TODO : hardcoded, shouldn't be

    static constexpr asset::vk_shader_data::layout_binding_info camera_layout_binding{
        .binding = 0,
        .set = 0,
        .count = 1,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER // TODO : push constant shall do too
    };
    static constexpr asset::vk_shader_data::layout_binding_info transforms_layout_binding{
        .binding = 1,
        .set = 0,
        .count = 1,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER // TODO : make it dynamic?
    };

    static constexpr std::array layout_bindings{ camera_layout_binding, transforms_layout_binding };
};

instanced_pass create_instanced_pass(const vkw::vk_device& device, u32_t frame_count);


}

#endif