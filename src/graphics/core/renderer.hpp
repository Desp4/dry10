#pragma once

#include "grinstance.hpp"

#include "vkw/pipeline_g.hpp"
#include "vkw/swapchain_p.hpp"

#include "material.hpp"

namespace gr::core
{
    class Renderer
    {
    public:
        constexpr static uint32_t IMAGE_COUNT = 4;

        Renderer(const GraphicsInstance* instance);

        vkw::GraphicsPipeline createPipeline(const Material& material, const vkw::DescriptorLayout& layout) const;

        std::pair<vkw::PresentFrame, const vkw::CmdBuffer*> beginFrame();
        void submitFrame(const vkw::PresentFrame& frameInfo);

    private:
        constexpr static VkFormat              DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
        constexpr static VkSampleCountFlagBits MSAA_SAMPLE_COUNT = VK_SAMPLE_COUNT_8_BIT;

        // TODO : do you even need instance?
        const GraphicsInstance* _instance;

        vkw::PresentSwapchain _swapchain;
        vkw::RenderPass _renderPass;

        vkw::Queue _presentQueue;
        std::array<vkw::CmdBuffer, IMAGE_COUNT> _cmdBuffers;

        VkExtent2D _surfaceExtent;
    };
}