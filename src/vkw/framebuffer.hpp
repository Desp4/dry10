#pragma once

#ifndef DRY_VK_FRAMEBUFFER_H
#define DRY_VK_FRAMEBUFFER_H

#include "vkw.hpp"

namespace dry::vkw {

class vk_framebuffer {
public:
    vk_framebuffer(VkRenderPass renderpass, std::span<const VkImageView> views, VkExtent2D extent);

    vk_framebuffer() = default;
    vk_framebuffer(vk_framebuffer&& oth) { *this = std::move(oth); }
    ~vk_framebuffer();

    VkFramebuffer handle() const { return _framebuffer; }

    vk_framebuffer& operator=(vk_framebuffer&&);

private:
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
};

}

#endif
