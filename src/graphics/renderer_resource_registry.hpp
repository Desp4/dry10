#pragma once

#ifndef DRY_GR_RENDERER_RESOURCE_REGISTRY_H
#define DRY_GR_RENDERER_RESOURCE_REGISTRY_H

#include "util/sparse_set.hpp"

#include "asset/vk_reflect.hpp"

#include "vkw/queue/queue_graphics.hpp"
#include "vkw/queue/queue_transfer.hpp"
#include "vkw/pipeline_g.hpp"
#include "vkw/texsampler.hpp"
#include "vkw/desc/desc_superpool.hpp"
#include "vkw/desc/desclayout.hpp"

namespace dry {

using layout_stage_mask_t = u32_t;
namespace layout_stage_mask {

constexpr layout_stage_mask_t none = 0x0;
constexpr layout_stage_mask_t all = 0xFFFFFFFF;

}

class renderer_resource_registry {
public:
    using index_type = u32_t;
    static constexpr index_type null_index = (std::numeric_limits<index_type>::max)();

    struct stage_descriptor_layout {
        std::vector<VkDescriptorSetLayoutBinding> exclude_bindings;
        VkDescriptorSetLayout layout;
        layout_stage_mask_t mask;
    };
    struct material_index {
        index_type pipeline;
        index_type material;
    };
    struct renderable_index {
        material_index material;
        index_type mesh;
        index_type renderable;
    };

    renderer_resource_registry() = default;
    renderer_resource_registry(
        const vkw::vk_device& device, const vkw::vk_render_pass& render_pass, // NOTE : only using allocator for plain graphics, so can have one universal render pass
        const vkw::vk_queue_graphics& graphics_queue, const vkw::vk_queue_transfer& transfer_queue,
        u32_t image_count
    );

    void set_descriptor_layout_stages(std::span<const stage_descriptor_layout> stage_layouts);
    void set_surface_extent(VkExtent2D extent); // TODO : can do a recreate on all pipelines if that happens
    // TODO : stage mask is for including specific stages
    index_type allocate_pipeline(const asset::shader_source& shader, layout_stage_mask_t stage_mask = layout_stage_mask::all);
    index_type allocate_vertex_buffer(const asset::mesh_source& mesh); // TODO : assuming {vec3 pos, vec2 uv}, need shader to deduce properly
    index_type allocate_texture(const asset::texture_source& texture);

    material_index allocate_material(index_type pipeline, std::span<const index_type> textures);
    renderable_index allocate_renderable(material_index material, index_type mesh);

    auto& pipeline_array() { return _pipelines; }
    auto& mesh_array() { return _vertex_buffers; }
    auto& descriptor_array() { return _pipeline_descriptors; }

private:
    struct renderable_data {
        std::vector<index_type> buffers;
        index_type descriptor; // per frame descriptor index
    };
    struct mesh_group {
        index_type mesh;
        sparse_set<renderable_data> renderables;
    };
    struct material_data {
        std::vector<index_type> textures;
        VkDescriptorSet descriptor;
        std::vector<mesh_group> mesh_groups;
    };
    struct pipeline_data {
        vkw::vk_pipeline_graphics pipeline;
        index_type layout;
        index_type descriptor_sets;
        sparse_set<material_data> materials;
        // don't hold reflection, it's always the same index
    };
    struct pipeline_descriptor_sets {
        vkw::vk_descriptor_superpool pool;
        std::vector<persistent_array<VkDescriptorSet>> frame_descriptors;
    };
    struct vertex_data {
        vkw::vk_buffer vertices;
        vkw::vk_buffer indices;
    };
    struct texture_data {
        vkw::vk_tex_sampler sampler;
        vkw::vk_image_view_pair texture;
    };

    index_type allocate_renderable_buffer(VkDescriptorType descriptor_type, VkDeviceSize range, VkDeviceSize offset);
    index_type allocate_renderable_descriptor(index_type pipeline, std::span<const index_type> buffers);

    const vkw::vk_device* _device;
    const vkw::vk_render_pass* _render_pass;
    const vkw::vk_queue_graphics* _graphics_queue;
    const vkw::vk_queue_transfer* _transfer_queue;

    u32_t _image_count;
    VkExtent2D _surface_extent;

    // pipeline creation descriptor layout
    std::vector<stage_descriptor_layout> _stage_layouts;

    // pipeline related info
    sparse_set<pipeline_data> _pipelines;
    persistent_array<vkw::vk_descriptor_layout> _layouts;
    persistent_array<asset::vk_shader_data> _pipeline_reflect_datas;
    persistent_array<pipeline_descriptor_sets> _pipeline_descriptors;
    // vertex buffers
    persistent_array<vertex_data> _vertex_buffers;
    // textures
    persistent_array<texture_data> _textures;
    // buffers
    std::vector<persistent_array<vkw::vk_buffer>> _per_frame_buffers;


    static constexpr u32_t _target_descriptor_pool_capacity = 128;
};

}

#endif