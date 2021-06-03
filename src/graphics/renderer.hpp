#pragma once

#ifndef DRY_GR_RENDERER_H
#define DRY_GR_RENDERER_H

#include <array>

#include <glm/mat4x4.hpp>

#include "window/window.hpp"

#include "vkw/device/instance.hpp"
#include "vkw/device/surface.hpp"
#include "vkw/swapchain_p.hpp"
#include "vkw/renderpass.hpp"

#include "renderer_resource_registry.hpp"

namespace dry {

class vulkan_renderer {
public:
    vulkan_renderer(const wsi::window& window);
    ~vulkan_renderer() { _device.wait_on_device(); }
    void submit_frame();

    auto& resource_registry() { return _resource_reg; }

private:
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
    void create_global_descriptors();

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

    renderer_resource_registry _resource_reg;

    std::vector<vkw::vk_cmd_buffer> _cmd_buffers;

    u32_t _image_count;

    // core global pipeline resources
    std::vector<vkw::vk_buffer> _model_mat_storage;
    std::vector<vkw::vk_buffer> _camera_ubos;
    std::vector<VkDescriptorSet> _global_descriptors;
    vkw::vk_descriptor_layout _global_descriptor_layout;
    vkw::vk_descriptor_pool _global_descriptor_pool;

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