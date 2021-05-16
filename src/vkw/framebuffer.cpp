#include "framebuffer.hpp"

#include "device/g_device.hpp"

namespace dry::vkw {

vk_framebuffer::vk_framebuffer(VkRenderPass renderPass, std::span<const VkImageView> views, VkExtent2D extent) {
    VkFramebufferCreateInfo framebuf_info{};
    framebuf_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuf_info.renderPass = renderPass;
    framebuf_info.attachmentCount = static_cast<uint32_t>(views.size());
    framebuf_info.pAttachments = views.data();
    framebuf_info.width = extent.width;
    framebuf_info.height = extent.height;
    framebuf_info.layers = 1;
    vkCreateFramebuffer(g_device->handle(), &framebuf_info, null_alloc, &_framebuffer);
}

vk_framebuffer::~vk_framebuffer() {
    vkDestroyFramebuffer(g_device->handle(), _framebuffer, null_alloc);
}

vk_framebuffer& vk_framebuffer::operator=(vk_framebuffer&& oth) {
    // destroy
    vkDestroyFramebuffer(g_device->handle(), _framebuffer, null_alloc);
    // move
    _framebuffer = oth._framebuffer;
    // null
    oth._framebuffer = VK_NULL_HANDLE;
    return *this;
}

}
