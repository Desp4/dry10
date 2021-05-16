#include "renderer.hpp"

#include "vkw/device/instance.hpp"
#include "vkw/device/g_device.hpp"

namespace dry::gr {

renderer::renderer(const graphics_instance& instance) {
    const auto capabilities = vkw::g_device->surface_capabilities(instance.surface().handle());
    _surface_extent = capabilities.currentExtent;
    _image_count = capabilities.maxImageCount == 0 ? capabilities.minImageCount + 1 :
        (std::min)(capabilities.minImageCount + 1, capabilities.maxImageCount);

    _swapchain = vkw::vk_swapchain_present(
        instance.surface().handle(), _surface_extent, capabilities.currentTransform, _image_count,
        graphics_instance::IMAGE_FORMAT, graphics_instance::IMAGE_COLORSPACE, graphics_instance::IMAGE_PRESENT_MODE
    );

    _render_pass = vkw::vk_render_pass(
        _surface_extent,
        vkw::render_pass_flags::color | vkw::render_pass_flags::depth | vkw::render_pass_flags::msaa,
        MSAA_SAMPLE_COUNT, graphics_instance::IMAGE_FORMAT, DEPTH_FORMAT
    );
    _render_pass.create_framebuffers(_swapchain.swap_views());

    const auto queue_indices = instance.present_queue();
    _present_queue = vkw::vk_queue(queue_indices.family_ind, queue_indices.queue_ind);

    _cmd_buffers.resize(_image_count);
    for (auto& buf : _cmd_buffers) {
        buf = vkw::vk_cmd_buffer(_present_queue.cmd_pool());
    }
}

vkw::vk_pipeline_graphics renderer::create_pipeline(const material& material, const vkw::vk_descriptor_layout& layout) const {
    std::vector<vkw::vk_shader_module> shader_modules;
    shader_modules.reserve(material.shader->oth_stages.size() + 1);
    shader_modules.emplace_back(material.shader->vert_stage.spirv, asset::shader_vk_stage(material.shader->vert_stage.stage));

    for (const auto& shader_bin : material.shader->oth_stages) {
        shader_modules.emplace_back(shader_bin.spirv, asset::shader_vk_stage(shader_bin.stage));
    }
    const auto vk_layout = layout.handle();
    return vkw::vk_pipeline_graphics(
        _render_pass, _surface_extent, shader_modules, asset::shader_vk_info(*material.shader),
        std::span{ &vk_layout, 1 }
    );
}

frame_context renderer::begin_frame() {
    const uint32_t frame_index = _swapchain.acquire_frame();
    const vkw::vk_cmd_buffer& buffer = _cmd_buffers[frame_index];
    vkResetCommandBuffer(buffer.handle(), 0);

    _render_pass.start_cmd_pass(buffer, frame_index);

    return { frame_index, &buffer };
}

void renderer::submit_frame(const frame_context& frame_ctx) {
    vkCmdEndRenderPass(frame_ctx.cmd_buf->handle());
    vkEndCommandBuffer(frame_ctx.cmd_buf->handle());

    _swapchain.submit_frame(_present_queue.handle(), frame_ctx.frame_index, frame_ctx.cmd_buf->handle());
}

}