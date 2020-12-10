#pragma once

#include "renderpass.hpp"
#include "material.hpp"
#include "framebuffer.hpp"
#include "desc/desclayout.hpp"
#include "desc/descsets.hpp"
#include "renderable.hpp"
#include "cmd/cmdbuffer.hpp"

namespace vkw
{
    class GraphicsPipeline : public Movable<GraphicsPipeline>
    {
    public:
        using Movable<GraphicsPipeline>::operator=;

        GraphicsPipeline() = default;
        GraphicsPipeline(GraphicsPipeline&&) = default;
        GraphicsPipeline(const Device* device, const RenderPass* renderPass, const Material& material, VkExtent2D extent);
        ~GraphicsPipeline();

        // TODO : perhaps split binding and drawing(one for pipeline, one for layout)
        void bindPipeline(VkCommandBuffer buffer) const;
        void bindDescriptorSets(VkCommandBuffer buffer, std::span<const VkDescriptorSet> sets) const;

    private:
        DevicePtr _device;
        NullablePtr<const RenderPass> _renderPass;

        VkHandle<VkPipelineLayout> _pipelineLayout;
        VkHandle<VkPipeline> _pipeline;
        
        bool _hasDepth;
        VkExtent2D _extent;
    };
}