#pragma once

#include "dab/dabasset.hpp"

#include "renderpass.hpp"
#include "shader/shader.hpp"

namespace dry::vkw {

class pipeline_graphics : public movable<pipeline_graphics> {
public:
    using movable<pipeline_graphics>::operator=;

    pipeline_graphics() = default;
    pipeline_graphics(pipeline_graphics&&) = default;
    pipeline_graphics(const render_pass& pass, VkExtent2D extent, std::span<const shader_module> modules,
                      const dab::shader_vk_data& shader_data, std::span<const VkDescriptorSetLayout> layouts);
    ~pipeline_graphics();

    void bind_pipeline(VkCommandBuffer buf) const;
    void bind_descriptor_sets(VkCommandBuffer buf, std::span<const VkDescriptorSet> sets) const;

private:
    vk_handle<VkPipelineLayout> _pipeline_layout;
    vk_handle<VkPipeline> _pipeline;
    // TODO : need extent and render_pass on reconstruction at say window resize
};

}