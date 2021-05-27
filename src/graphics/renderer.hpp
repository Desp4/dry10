#pragma once

#ifndef DRY_GR_RENDERER_H
#define DRY_GR_RENDERER_H

#include <array>
#include <unordered_map>

#include <glm/mat4x4.hpp>

#include "util/sparse_set.hpp"

#include "window/window.hpp"

#include "vkw/device/instance.hpp"
#include "vkw/device/surface.hpp"
#include "vkw/swapchain_p.hpp"
#include "vkw/renderpass.hpp"
#include "vkw/queue/queue_graphics.hpp"
#include "vkw/queue/queue_transfer.hpp"
#include "vkw/desc/desclayout.hpp"
#include "vkw/desc/desc_superpool.hpp"
#include "vkw/pipeline_g.hpp"
#include "vkw/texsampler.hpp"

#include "asset/vk_reflect.hpp"

namespace dry {

struct material_asset {
    const asset::shader_asset* shader;
    std::vector<const asset::texture_asset*> textures;
};

class vulkan_renderer {
private:
    template<typename Handle>
    using asset_hash_map = std::unordered_map<asset::hash_t, Handle>;
    using shader_hash = asset::hash_t;
    using mesh_hash = asset::hash_t;
    using tex_hash = asset::hash_t;
    using buffer_hash = persistent_index_type;

    struct material_hash {
        material_hash() = default;
        material_hash(const material_asset& material) {
            tex_hs.reserve(material.textures.size());
            for (auto tex : material.textures) {
                tex_hs.push_back(tex->hash);
            }
        }

        std::vector<asset::hash_t> tex_hs;

        bool operator==(const material_hash& oth) const {
            if (tex_hs.size() != oth.tex_hs.size()) {
                return false;
            }
            // can do a memcmp
            for (auto i = 0u; i < tex_hs.size(); ++i) {
                if (tex_hs[i] != oth.tex_hs[i]) {
                    return false;
                }
            }
            return true;
        }
    };
    struct material_hasher {
        // TODO : dumb, and the hashing is pretty bad
        size_t operator()(const material_hash& val) const {
            size_t hashed_texs = 0;
            for (auto tex_h : val.tex_hs) {
                hashed_texs ^= static_cast<size_t>(tex_h);
            }
            return hashed_texs;
        }
    };
    // TODO : same for this, that is NOT a hash value, that is bullshit
    struct renderable_hash {
        shader_hash shader;
        material_hash material;
        mesh_hash mesh;
        persistent_index_type index;

        bool operator==(const renderable_hash&) const = default;
    };
    struct renderable_hasher {
        size_t operator()(const renderable_hash& val) const {
            size_t index_hash = static_cast<size_t>(val.index);
            size_t rest_hash = (static_cast<size_t>(val.shader) << (64 - 8));
            rest_hash &= (material_hasher{}(val.material) << (64 - 24));
            rest_hash &= (static_cast<size_t>(val.mesh) << (64 - 40));
            return index_hash & rest_hash;
        }
    };

    struct renderable {
        persistent_index_type desc_h;
    };

    struct queue_family_info {
        u32_t family_ind;
        u32_t queue_ind;
    };
    struct populated_queue_info {
        std::array<queue_family_info, 3> queue_init_infos;
        std::vector<vkw::queue_info> device_queue_infos;
    };
    struct material_data {
        std::vector<tex_hash> texs;
        VkDescriptorSet descriptor;

        std::unordered_map<mesh_hash, sparse_set<renderable>> renderables;
    };
    struct shader_pipeline {
        vkw::vk_pipeline_graphics pipeline;
        asset::vk_shader_data shader_data;

        vkw::vk_descriptor_layout layout; // will be default dummy if no additional bindings
        vkw::vk_descriptor_superpool descpool; // and this too, TODO : move them to some other container?
        std::vector<persistent_array<VkDescriptorSet>> renderable_descs; // per frame descriptors

        // material grouped per pipeline
        std::unordered_map<material_hash, material_data, material_hasher> materials;
    };
    struct vertex_buffer {
        vkw::vk_buffer vertex;
        vkw::vk_buffer index;
    };
    struct tex_sampler {
        vkw::vk_tex_sampler sampler;
        vkw::vk_image_view_pair texture;
    };


public:
    vulkan_renderer(const wsi::window& window);
    ~vulkan_renderer() { _device.wait_on_device(); }

    renderable_hash create_renderable(const asset::mesh_asset& mesh, const material_asset& material);
    void submit_frame();

private:
    // init functions
    VkPhysicalDevice find_physical_device();
    // return queue infos and and family-index pair for each used queue
    populated_queue_info populate_queue_infos(VkPhysicalDevice phys_device);
    void create_global_descriptors();
    // allocators
    VkDescriptorSet create_material_descriptor(shader_pipeline& pipeline, const material_data& mat_data);
    persistent_index_type create_renderable_frame_descriptors(shader_pipeline& pipeline, std::span<const buffer_hash> buffers);

    shader_pipeline create_pipeline(const asset::shader_source& shader);
    vertex_buffer create_vertex_buffers(const asset::mesh_source& mesh);
    tex_sampler create_sampler(const asset::texture_source& texture);
    buffer_hash create_buffer(const asset::vk_shader_data& shader_data, const decltype(shader_data.buffer_infos)::value_type& buffer_info);

    static const vkw::vk_instance& vk_instance();
    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* user_data
    );
    // core renderer structs
    vkw::vk_surface _surface;
    vkw::vk_device _device;

    vkw::vk_swapchain_present _swapchain;
    vkw::vk_render_pass _render_pass;

    vkw::vk_queue _present_queue;
    vkw::vk_queue_graphics _graphics_queue;
    vkw::vk_queue_transfer _transfer_queue;

    std::vector<vkw::vk_cmd_buffer> _cmd_buffers;

    u32_t _image_count;

    // core global pipeline resources
    std::vector<vkw::vk_buffer> _model_mat_storage;
    std::vector<vkw::vk_buffer> _camera_ubos;
    std::vector<VkDescriptorSet> _global_descriptors;
    vkw::vk_descriptor_layout _global_descriptor_layout;
    vkw::vk_descriptor_pool _global_descriptor_pool;

    // asset hash lookup
    asset_hash_map<vertex_buffer> _meshes;
    asset_hash_map<tex_sampler> _samplers;
    asset_hash_map<shader_pipeline> _pipelines;
    
    // per frame allocated structures
    // persistent_array allocations will return matching indicies for identical allocation patterns
    std::vector<persistent_array<vkw::vk_buffer>> _buffers;

    std::unordered_map<renderable_hash, std::vector<buffer_hash>, renderable_hasher> _renderable_buffer_map;

    // constants
    static constexpr std::array<const char*, 1> _device_extensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    static constexpr VkPhysicalDeviceFeatures _device_features{
        .sampleRateShading = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };
    static constexpr VkFormat _primary_image_format = VK_FORMAT_B8G8R8A8_SRGB;
    static constexpr VkFormat _primary_depth_format = VK_FORMAT_D32_SFLOAT;
    static constexpr VkColorSpaceKHR _primary_image_colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    static constexpr VkPresentModeKHR _primary_image_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    static constexpr VkSampleCountFlagBits _primary_msaa_sample_count = VK_SAMPLE_COUNT_8_BIT;
    static constexpr u32_t _primary_desc_pool_capacity = 128;

    // global pipeline constants and types
    struct camera_transform {
        glm::mat4 viewproj;
        glm::mat4 proj;
        glm::mat4 view;
    };
    using model_transform = glm::mat4;

    static constexpr VkDescriptorSetLayoutBinding _camera_data_layout_binding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };
    static constexpr VkDescriptorSetLayoutBinding _model_data_layout_binding{
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };
    static constexpr u32_t _primary_model_mat_storage_size = 4096;
};

}

#endif