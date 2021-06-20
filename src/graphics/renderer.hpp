#pragma once

#ifndef DRY_GR_RENDERER_H
#define DRY_GR_RENDERER_H

#include <unordered_map>

#include "util/sparse_set.hpp"

#include "window/window.hpp"

#include "vkw/device/instance.hpp"
#include "vkw/device/surface.hpp"
#include "vkw/desc/desc_superpool.hpp"
#include "vkw/queue/queue_graphics.hpp"
#include "vkw/queue/queue_transfer.hpp"
#include "vkw/swapchain_p.hpp"

#include "vkw/pipeline_g.hpp"

#include "unshaded_pass.hpp"

namespace dry {

struct renderer_resources {
    using resource_id = persistent_index_type;

    struct vertex_buffer {
        vkw::vk_buffer vertices;
        vkw::vk_buffer indices;
    };
    struct material {
        resource_id pipeline;
        std::vector<resource_id> textures;
        // TODO : no uniforms per material, use an SSBO
    };
    struct renderable {
        const object_transform* transform_ptr;
        resource_id material;
    };
    struct shader_pipeline {
        vkw::vk_pipeline_graphics pipeline;
        vkw::vk_descriptor_layout layout;
        // NOTE : not sharing pools, a TODO for later times
        vkw::vk_descriptor_superpool descriptor_pool;
        // TODO : don't contain anything
        std::vector<VkDescriptorSet> descriptors;
        // TODO : nothing here either
        std::vector<std::vector<vkw::vk_buffer>> uniform_buffers;

        std::unordered_map<resource_id, sparse_set<renderable>> renderables;
    };
    
    persistent_array<vertex_buffer> vertex_buffers;
    persistent_array<vkw::vk_image_view_pair> textures;
    persistent_array<material> materials;
    sparse_set<shader_pipeline> pipelines;

    // reference counters
    using refcount_array = persistent_array<u32_t>;
    refcount_array vertex_refcount;
    refcount_array texture_refcount;
    refcount_array material_refcount;
    refcount_array pipeline_refcount;
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
    // referencing types
    // TODO : limited, don't have anything other than textures yet
    resource_id create_material(resource_id pipeline, std::span<const resource_id> textures);
    renderable_id create_renderable(resource_id material, resource_id mesh);

    void destroy_renderable(renderable_id rend);

    void bind_renderable_transform(renderable_id rend, const object_transform& transform);

    // TODO : public deleted resources, doing nothing currently

private:
    // internal types for init, not used after
    struct queue_family_info {
        u32_t family_ind;
        u32_t queue_ind;
    };
    struct populated_queue_info {
        std::array<queue_family_info, 3> queue_init_infos;
        std::vector<vkw::queue_info> device_queue_infos;
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

    // render passes
    unshaded_pass _unshaded_pass;

    u32_t _image_count;
    VkExtent2D _extent;

    static constexpr object_transform _default_transform{
        .model = glm::mat4{ 1.0f }
    };

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
};

}

#endif