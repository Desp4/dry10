#pragma once

#ifndef DRY_VK_PIPELINE_G_H
#define DRY_VK_PIPELINE_G_H

#include "renderpass.hpp"
#include "shader/shader.hpp"

namespace dry::vkw {

struct g_pipeline_create_ctx {
    VkPolygonMode fill_mode = VK_POLYGON_MODE_FILL;
};

class vk_pipeline_graphics {
public:
    vk_pipeline_graphics(const vk_device& device,
        const vk_render_pass& pass, VkExtent2D extent, std::span<const vk_shader_module> modules,
        std::span<const VkVertexInputBindingDescription> vertex_bindings, std::span<const VkVertexInputAttributeDescription> vertex_attributes,
        std::span<const VkDescriptorSetLayout> layouts, const g_pipeline_create_ctx& ctx
    );

    vk_pipeline_graphics() = default;
    vk_pipeline_graphics(vk_pipeline_graphics&& oth) { *this = std::move(oth); }

    ~vk_pipeline_graphics();

    void bind_pipeline(VkCommandBuffer buf) const;

    VkPipelineLayout layout() const { return _pipeline_layout; }

    vk_pipeline_graphics& operator=(vk_pipeline_graphics&&);

private:
    const vk_device* _device = nullptr;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    // TODO : need extent and render_pass on reconstruction at say window resize
};

}

#endif
