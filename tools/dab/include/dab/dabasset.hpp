#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "asset.hpp"

namespace dab {

struct mesh {
    static constexpr asset_type type = asset_type::mesh;

    struct tex_coord_set {
        std::vector<uint8_t> tex_data;
        uint8_t component_count;
    };

    std::vector<uint8_t> vertex_data;
    std::vector<tex_coord_set> tex_coord_sets;
};

struct texture {
    static constexpr asset_type type = asset_type::texture;

    uint8_t channels;
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> pixel_data;
};

struct shader_vk_data {
    template<typename T>
    struct descriptor_info {
        uint32_t binding_ind;
        T info;
    };

    VkVertexInputBindingDescription vertex_binding;
    std::vector<VkVertexInputAttributeDescription> vertex_descriptors;

    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;

    std::vector<descriptor_info<VkDescriptorBufferInfo>> buffer_infos;
    std::vector<descriptor_info<VkDescriptorImageInfo>> comb_sampler_infos;
};

struct shader {
    static constexpr asset_type type = asset_type::shader;

    struct shader_module {
        VkShaderStageFlagBits stage;
        std::vector<uint32_t> module_data;
    };

    std::vector<shader_module> modules;

    shader_vk_data vk_data() const;
};

}