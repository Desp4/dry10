#pragma once

#include <span>

#include "vkw.hpp"

namespace dry::vkw {

class framebuffer : public movable<framebuffer> {
public:
    using movable<framebuffer>::operator=;

    framebuffer() = default;
    framebuffer(framebuffer&&) = default;
    framebuffer(VkRenderPass renderpass, std::span<const VkImageView> views, VkExtent2D extent);
    ~framebuffer();

    const VkFramebuffer& buffer() const {
        return _framebuffer;
    }

private:
    vk_handle<VkFramebuffer> _framebuffer;
};

}