#pragma once

#ifndef DRY_GR_RENDERER_H
#define DRY_GR_RENDERER_H

#include <unordered_map>
#include <memory>

#include "util/sparse_set.hpp"

#include "window/window.hpp"

#include "vkw/device/instance.hpp"
#include "vkw/device/surface.hpp"
#include "vkw/swapchain_p.hpp"

#include "vkw/pipeline_g.hpp"

#include "instanced_pass.hpp"
#include "pipeline_resources.hpp"
#include "texarr.hpp"
#include "material_base.hpp"

namespace dry {

struct renderer_resources {
    using resource_id = persistent_index_type;

    struct vertex_buffer {
        vkw::vk_buffer vertices;
        vkw::vk_buffer indices;
    };
    using renderable = instanced_pass::instance_input;
    struct shader_pipeline {   
        vkw::vk_pipeline_graphics pipeline;
        pipeline_resources pipeline_data;

        std::vector<std::vector<VkDescriptorSet>> shared_descriptors;
        std::unordered_map<resource_id, sparse_set<renderable>> renderables;

        sparse_set<resource_id> material_inds; // TODO : too much redundant info
        std::vector<bool> material_update_status;
    };
    
    persistent_array<vertex_buffer> vertex_buffers;
    persistent_array<vkw::vk_image_view_pair> textures;
    persistent_array<std::unique_ptr<material_base>> materials;
    sparse_set<shader_pipeline> pipelines;

    // reference counters
    using refcount_array = persistent_array<u32_t>;

    camera_transform cam_transform;
};

class vulkan_renderer {
public:
    using resource_id = renderer_resources::resource_id;
    struct renderable_id {
        u32_t renderable;
        u16_t pipeline;
        u16_t mesh;
    };

    vulkan_renderer(const wsi::window& window);
    ~vulkan_renderer() { _device.wait_on_device(); }
    void submit_frame();

    resource_id create_texture(const asset::texture_source& tex);
    resource_id create_mesh(const asset::mesh_asset& mesh);
    resource_id create_shader(const asset::shader_asset& shader);

    template<typename Material, typename... Ts>
    resource_id create_material(resource_id pipeline, Ts&&... args);
    renderable_id create_renderable(resource_id material, resource_id mesh);

    void destroy_renderable(renderable_id rend);

    void update_renderable_transform(renderable_id rend, const object_transform& trans);
    void update_camera_transform(const camera_transform& trans);

    template<typename T>
    void write_buffer(resource_id pipeline, u32_t binding, const T& value);
    template<typename T>
    void write_buffer(resource_id pipeline, u32_t binding, const std::vector<T>& values);
    template<typename T>
    void write_buffer(resource_id pipeline, u32_t binding, std::span<const T> values);

private:
    friend class pipeline_base;
    // internal types for init, not used after
    struct queue_family_info {
        u32_t family_ind;
        u32_t queue_ind;
    };
    struct populated_queue_info {
        std::array<queue_family_info, 3> queue_init_infos;
        std::vector<vkw::queue_info> device_queue_infos;
    };
    struct buffer_write_job {
        std::vector<std::byte> data;
        resource_id pipeline;
        u32_t binding;
    };

    // init functions
    VkPhysicalDevice find_physical_device();
    // return queue infos and and family-index pair for each used queue
    populated_queue_info populate_queue_infos(VkPhysicalDevice phys_device);

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

    renderer_resources _resources;

    instanced_pass _instanced_pass;

    texture_array _texarr;

    std::vector<std::vector<buffer_write_job>> _buffer_write_queues;

    u32_t _image_count;
    VkExtent2D _extent;

    static constexpr object_transform _default_transform{
        .model = glm::mat4{ 1.0f }
    };
    static constexpr camera_transform _default_cam_transform{};

    // constants
    static constexpr std::array _device_extensions{
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
    static constexpr u32_t _primary_descriptor_pool_capacity = 128;

    static constexpr u32_t _default_tex_mip_levels = 4;
};


template<typename Material, typename... Ts>
vulkan_renderer::resource_id vulkan_renderer::create_material(resource_id pipeline, Ts&&... args) {
    auto& pipeline_res = _resources.pipelines[pipeline];
    // assert(pipeline_res.pipeline_data.has_materials()); NOTE : still using dummy materials even if no info in renderable creation

    const auto local_ind = pipeline_res.material_inds.emplace(0);
    const auto ind = _resources.materials.emplace(std::make_unique<Material>(std::forward<Ts>(args)...));

    auto& base_material = *_resources.materials[ind];
    base_material.pipeline_index = pipeline;
    base_material.local_index = local_ind;

    pipeline_res.material_inds[local_ind] = ind;
    for (auto i = 0u; i < pipeline_res.material_update_status.size(); ++i) {
        pipeline_res.material_update_status[i] = false;
    }

    return static_cast<resource_id>(ind);
}

template<typename T>
void vulkan_renderer::write_buffer(resource_id pipeline, u32_t binding, const T& value) {
    write_buffer(pipeline, binding, std::span{ &value, 1 });
}
template<typename T>
void vulkan_renderer::write_buffer(resource_id pipeline, u32_t binding, const std::vector<T>& values) {
    write_buffer(pipeline, binding, std::span{ values.begin(), values.end() });
}
template<typename T>
void vulkan_renderer::write_buffer(resource_id pipeline, u32_t binding, std::span<const T> values) {
    for (auto& frame_jobs : _buffer_write_queues) {
        buffer_write_job job{ .pipeline = pipeline, .binding = binding };
        job.data.resize(values.size_bytes());
        std::copy(values.begin(), values.end(), reinterpret_cast<T*>(job.data.data()));

        frame_jobs.push_back(std::move(job));
    }
}

}

#endif