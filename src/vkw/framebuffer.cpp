#include "framebuffer.hpp"

namespace vkw
{
    FrameBuffer::FrameBuffer(const Device* device, VkRenderPass renderPass, std::span<const VkImageView> views, VkExtent2D extent) :
        _device(device)
    {
        VkFramebufferCreateInfo framebufInfo{};
        framebufInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufInfo.renderPass = renderPass;
        framebufInfo.attachmentCount = views.size();
        framebufInfo.pAttachments = views.data();
        framebufInfo.width = extent.width;
        framebufInfo.height = extent.height;
        framebufInfo.layers = 1;
        vkCreateFramebuffer(_device->device(), &framebufInfo, NULL_ALLOC, &_frameBuffer);
    }

    FrameBuffer::~FrameBuffer()
    {
        if (_device) vkDestroyFramebuffer(_device->device(), _frameBuffer, NULL_ALLOC);
    }
}