#pragma once

#ifndef DRY_VK_PIPELINE_G_H
#define DRY_VK_PIPELINE_G_H

#include "asset/vk_reflect.hpp"

#include "renderpass.hpp"
#include "shader/shader.hpp"

namespace dry::vkw {

class vk_pipeline_graphics {
public:
    vk_pipeline_graphics(
        const vk_render_pass& pass, VkExtent2D extent, std::span<const vk_shader_module> modules,
        const asset::vk_shader_data& shader_data, std::span<const VkDescriptorSetLayout> layouts
    );

    vk_pipeline_graphics() = default;
    vk_pipeline_graphics(vk_pipeline_graphics&& oth) { *this = std::move(oth); }

    ~vk_pipeline_graphics();

    void bind_pipeline(VkCommandBuffer buf) const;
    void bind_descriptor_sets(VkCommandBuffer buf, std::span<const VkDescriptorSet> sets) const;

    vk_pipeline_graphics& operator=(vk_pipeline_graphics&&);

private:
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    // TODO : need extent and render_pass on reconstruction at say window resize
};

}

#endif
