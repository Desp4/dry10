#pragma once

#ifndef DRY_GR_TEXARR_H
#define DRY_GR_TEXARR_H

#include "asset/vk_reflect.hpp"

#include "vkw/queue/queue_graphics.hpp"
#include "vkw/desc/desclayout.hpp"
#include "vkw/desc/descpool.hpp"
#include "vkw/texsampler.hpp"
#include "vkw/image/imageviewpair.hpp"

namespace dry {

struct texture_array {
    vkw::vk_tex_sampler sampler;
    vkw::vk_image_view_pair dummy_image;
    std::vector<VkDescriptorImageInfo> texture_array_infos;

    std::vector<VkDescriptorSet> texarr_descriptors;
    std::vector<bool> texture_update_status;

    vkw::vk_descriptor_layout texarr_descriptor_layout;
    vkw::vk_descriptor_pool texarr_descriptor_pool;

    void add_texture(u32_t index, VkImageView texture);
    void remove_texture(u32_t index);
    void update_descriptors(const vkw::vk_device& device, u32_t frame);

    static constexpr u32_t sampler_mip_levels = 4;
    static constexpr u32_t array_size = 512;

    static constexpr asset::vk_shader_data::layout_binding_info sampler_layout_binding{
        .binding = 0,
        .set = 1,
        .count = 1,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .type = VK_DESCRIPTOR_TYPE_SAMPLER
    };
    static constexpr asset::vk_shader_data::layout_binding_info texarr_layout_binding{
        .binding = 1,
        .set = 1,
        .count = array_size,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    };

    static constexpr std::array layout_bindings{ sampler_layout_binding, texarr_layout_binding };
};

texture_array create_texture_array(const vkw::vk_device& device, const vkw::vk_queue_graphics& graphics_queue, u32_t frame_count);

}

#endif