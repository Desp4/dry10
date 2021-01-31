#pragma once

#include "dab/import.hpp"

#include "renderpass.hpp"
#include "shader/shader.hpp"
#include "desc/desclayout.hpp"
#include "cmd/cmdbuffer.hpp"

namespace vkw
{
    class GraphicsPipeline : public Movable<GraphicsPipeline>
    {
    public:
        using Movable<GraphicsPipeline>::operator=;

        GraphicsPipeline() = default;
        GraphicsPipeline(GraphicsPipeline&&) = default;
        GraphicsPipeline(const Device* device, const RenderPass* renderPass, VkExtent2D extent,
                         std::span<const ShaderModule> modules, const dab::ShaderVkData& shaderData, std::span<const VkDescriptorSetLayout> layouts);
        ~GraphicsPipeline();

        // TODO : perhaps split binding and drawing(one for pipeline, one for layout)
        void bindPipeline(VkCommandBuffer buffer) const;
        void bindDescriptorSets(VkCommandBuffer buffer, std::span<const VkDescriptorSet> sets) const;

    private:
        DevicePtr _device;
        NullablePtr<const RenderPass> _renderPass;

        VkHandle<VkPipelineLayout> _pipelineLayout;
        VkHandle<VkPipeline> _pipeline;
        
        VkExtent2D _extent;
    };
}