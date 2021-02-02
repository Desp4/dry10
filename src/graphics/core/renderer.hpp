#pragma once

#include "grinstance.hpp"

#include "vkw/pipeline_g.hpp"
#include "vkw/swapchain_p.hpp"

#include "material.hpp"

namespace gr::core
{
    struct RenderContext
    {
        uint32_t frameIndex;
        const vkw::CmdBuffer* cmdBuffer;
    };

    class Renderer
    {
    public:
        Renderer(const GraphicsInstance* instance);

        vkw::GraphicsPipeline createPipeline(const Material& material, const vkw::DescriptorLayout& layout) const;

        RenderContext beginFrame();
        void submitFrame(uint32_t frameIndex);

        uint32_t imageCount() const { return _imageCount; }

    private:
        constexpr static VkFormat              DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
        constexpr static VkSampleCountFlagBits MSAA_SAMPLE_COUNT = VK_SAMPLE_COUNT_8_BIT;

        // TODO : do you even need instance?
        const GraphicsInstance* _instance;

        vkw::PresentSwapchain _swapchain;
        vkw::RenderPass _renderPass;

        vkw::Queue _presentQueue;
        std::vector<vkw::CmdBuffer> _cmdBuffers;

        uint32_t _imageCount;
        VkExtent2D _surfaceExtent;
    };
}