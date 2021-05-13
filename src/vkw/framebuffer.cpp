#include "framebuffer.hpp"
#include "device/device.hpp"

namespace dry::vkw {

framebuffer::framebuffer(VkRenderPass renderPass, std::span<const VkImageView> views, VkExtent2D extent) {
    VkFramebufferCreateInfo framebuf_info{};
    framebuf_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuf_info.renderPass = renderPass;
    framebuf_info.attachmentCount = static_cast<uint32_t>(views.size());
    framebuf_info.pAttachments = views.data();
    framebuf_info.width = extent.width;
    framebuf_info.height = extent.height;
    framebuf_info.layers = 1;
    vkCreateFramebuffer(device_main::device(), &framebuf_info, NULL_ALLOC, &_framebuffer);
}

framebuffer::~framebuffer() {
    vkDestroyFramebuffer(device_main::device(), _framebuffer, NULL_ALLOC);
}

}