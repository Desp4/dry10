#pragma once

#include <vector>

#include "image/imageviewpair.hpp"
#include "framebuffer.hpp"

namespace vkw
{
    enum RenderPassFlagBits : uint32_t
    {
        RenderPass_Color = 0x1,
        RenderPass_Depth = 0x2,
        RenderPass_MSAA = 0x4
    };
    using RenderPassFlags = uint32_t;

    class RenderPass : Movable<RenderPass>
    {
    public:
        using Movable<RenderPass>::operator=;

        RenderPass() = default;
        RenderPass(RenderPass&&) = default;
        RenderPass(const Device* device, VkExtent2D extent, RenderPassFlags flags,
                   VkFormat imageFormat, VkFormat depthFormat, VkSampleCountFlagBits samples);
        ~RenderPass();

        void createFrameBuffers(std::span<const ImageView> swapViews);

        void startCmdPass(VkCommandBuffer buffer, uint32_t frameInd) const;

        VkRenderPass renderPass() const;
        VkSampleCountFlagBits rasterSampleCount() const;
        VkBool32 depthEnabled() const;

    private:
        DevicePtr _device;

        VkHandle<VkRenderPass> _pass;

        VkExtent2D _extent;
        VkBool32 _depthEnabled;
        VkSampleCountFlagBits _samples;

        ImageViewPair _depthImage;
        ImageViewPair _colorImage;

        std::vector<FrameBuffer> _frameBuffers;
    };
}