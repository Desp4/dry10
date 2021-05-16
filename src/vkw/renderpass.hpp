#pragma once

#ifndef DRY_VK_RENDERPASS_H
#define DRY_VK_RENDERPASS_H

#include "image/imageviewpair.hpp"
#include "framebuffer.hpp"
#include "cmd/cmdbuffer.hpp"

namespace dry::vkw {

using render_pass_flag = u8_t;
namespace render_pass_flags {

constexpr render_pass_flag color = 0x1;
constexpr render_pass_flag depth = 0x2;
constexpr render_pass_flag msaa = 0x4;

}

class vk_render_pass {
public:
    vk_render_pass(
        VkExtent2D extent, render_pass_flag flags, VkSampleCountFlagBits samples,
        VkFormat image_format, VkFormat depth_format
    );

    vk_render_pass() = default;
    vk_render_pass(vk_render_pass&& oth) { *this = std::move(oth); }
    ~vk_render_pass();

    void create_framebuffers(std::span<const vk_image_view> swap_views);
    void start_cmd_pass(const vk_cmd_buffer& buf, u32_t frame_ind) const;

    VkRenderPass handle() const { return _pass; }
    VkSampleCountFlagBits raster_sample_count() const { return _samples; }
    bool depth_enabled() const { return _depth_enabled; }

    vk_render_pass& operator=(vk_render_pass&&);

private:
    VkRenderPass _pass = VK_NULL_HANDLE;

    vk_image_view_pair _depth_image;
    vk_image_view_pair _color_image;

    std::vector<vk_framebuffer> _framebuffers;

    VkExtent2D _extent;
    VkSampleCountFlagBits _samples;
    bool _depth_enabled;
};

}

#endif
