#pragma once

#include "grinstance.hpp"
#include "material.hpp"

#include "vkw/pipeline_g.hpp"
#include "vkw/swapchain_p.hpp"
#include "vkw/desc/desclayout.hpp"

namespace dry::gr {

struct frame_context {
    uint32_t frame_index;
    const vkw::cmd_buffer* cmd_buf;
};

class renderer {
public:
    renderer(const graphics_instance& instance);

    vkw::pipeline_graphics create_pipeline(const material& material, const vkw::descriptor_layout& layout) const;

    frame_context begin_frame();
    void submit_frame(const frame_context& frame_ctx);

    uint32_t image_count() const {
        return _image_count;
    }

private:
    constexpr static VkFormat              DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
    constexpr static VkSampleCountFlagBits MSAA_SAMPLE_COUNT = VK_SAMPLE_COUNT_8_BIT;

    vkw::swapchain_present _swapchain;
    vkw::render_pass _render_pass;

    vkw::queue_base _present_queue;
    std::vector<vkw::cmd_buffer> _cmd_buffers;

    uint32_t _image_count;
    VkExtent2D _surface_extent;
};

}