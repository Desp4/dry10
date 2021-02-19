#pragma once

#include <vector>

#include "image/imageviewpair.hpp"
#include "framebuffer.hpp"
#include "cmd/cmdbuffer.hpp"

namespace dry::vkw {

enum class render_pass_flags : uint32_t {
    color = 0x1,
    depth = 0x2,
    msaa  = 0x4
};

consteval bool enable_flags(render_pass_flags) {
    return true;
}

class render_pass : public movable<render_pass> {
public:
    using movable<render_pass>::operator=;

    render_pass() = default;
    render_pass(render_pass&&) = default;
    render_pass(VkExtent2D extent, render_pass_flags flags, VkSampleCountFlagBits samples,
                VkFormat image_format, VkFormat depth_format);
    ~render_pass();

    void create_framebuffers(std::span<const image_view> swap_views);
    void start_cmd_pass(const cmd_buffer& buf, uint32_t frame_ind) const;

    const VkRenderPass& pass() const {
        return _pass;
    }
    VkSampleCountFlagBits raster_sample_count() const {
        return _samples;
    }
    VkBool32 depth_enabled() const {
        return _depth_enabled;
    }

private:
    vk_handle<VkRenderPass> _pass;

    VkExtent2D _extent;
    VkBool32 _depth_enabled;
    VkSampleCountFlagBits _samples;

    image_view_pair _depth_image;
    image_view_pair _color_image;

    std::vector<framebuffer> _framebuffers;
};

}
