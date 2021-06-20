#pragma once

#ifndef DRY_GR_UNSHADED_PASS_H
#define DRY_GR_UNSHADED_PASS_H

#include <array>

#include <glm/mat4x4.hpp>

#include "vk_initers.hpp"

#include "vkw/buffer.hpp"
#include "vkw/desc/descpool.hpp"
#include "vkw/desc/desclayout.hpp"
#include "vkw/texsampler.hpp"
#include "vkw/image/imageviewpair.hpp"

#include "vkw/queue/queue_transfer.hpp"
#include "vkw/queue/queue_graphics.hpp"

namespace dry {

struct camera_transform {
    glm::mat4 viewproj;
    glm::mat4 proj;
    glm::mat4 view;
};

struct object_transform {
    glm::mat4 model;
};

struct unshaded_pass {
    struct instance_input {
        object_transform transform;
        u32_t tex_index;
    };

    // per frame
    std::vector<vkw::vk_buffer> camera_transforms;
    std::vector<VkDescriptorSet> unshaded_descriptors;
    std::vector<bool> texture_update_status; // true - up to date

    std::vector<vkw::vk_buffer> instance_buffers;
    vkw::vk_buffer instance_staging_buffer;

    vkw::vk_descriptor_layout unshaded_descriptor_layout;
    vkw::vk_descriptor_pool unshaded_descriptor_pool;

    vkw::vk_tex_sampler sampler;

    // texture_array_size in size, just a tad to big for the stack
    std::vector<VkDescriptorImageInfo> texture_array_infos;
    // dummy image for filling in empty textures in array
    vkw::vk_image_view_pair dummy_image;

    static constexpr u32_t mip_levels = 8;
    static constexpr u32_t texture_array_size = 32;
    static constexpr u32_t combined_instance_buffer_count = 4096 * 8; // TODO : hardcoded, shouldn't be
    // not counting sampler in fragment, set by each pipeline
    static constexpr asset::vk_shader_data::layout_binding_info camera_layout_binding{
        .binding = 0,
        .set = 0,
        .count = 1,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    };
    static constexpr asset::vk_shader_data::layout_binding_info sampler_layout_binding{
        .binding = 1,
        .set = 0,
        .count = 1,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .type = VK_DESCRIPTOR_TYPE_SAMPLER
    };
    static constexpr asset::vk_shader_data::layout_binding_info texarr_layout_binding{
        .binding = 2,
        .set = 0,
        .count = texture_array_size,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    };

    static constexpr std::array layout_bindings{ camera_layout_binding, sampler_layout_binding, texarr_layout_binding };

    static constexpr vertex_input_setting mesh_vertex_input{
        .binding_description = VkVertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(asset::mesh_source::vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX },
        .first_location = 0
    };
    static constexpr vertex_input_setting instance_vertex_input{
        .binding_description = VkVertexInputBindingDescription {
            .binding = 1,
            .stride = sizeof(instance_input),
            .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE },
        .first_location = 3
    };

    static constexpr std::array vertex_inputs{ mesh_vertex_input, instance_vertex_input };
};

unshaded_pass create_unshaded_pass(const vkw::vk_device& device, const vkw::vk_queue_graphics& graphics_queue, u32_t frame_count);

// TODO : no batch updates yet
void update_unshaded_textures(unshaded_pass& pass, u32_t index, VkImageView view);
void update_texture_descriptors(const vkw::vk_device& device, unshaded_pass& pass, u32_t frame);


}

#endif