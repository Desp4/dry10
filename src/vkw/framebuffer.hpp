#pragma once

#include "device/device.hpp"

namespace vkw
{
    class FrameBuffer : public Movable<FrameBuffer>
    {
    public:
        using Movable<FrameBuffer>::operator=;

        FrameBuffer() = default;
        FrameBuffer(FrameBuffer&&) = default;
        FrameBuffer(const Device* device, VkRenderPass renderPass, std::span<const VkImageView> views, VkExtent2D extent);
        ~FrameBuffer();
        
        VkFramebuffer frameBuffer() const;

    private:
        DevicePtr _device;

        VkHandle<VkFramebuffer> _frameBuffer;
    };
}