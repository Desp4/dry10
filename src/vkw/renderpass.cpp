#include <array>

#include "renderpass.hpp"
#include "device/device.hpp"

namespace dry::vkw {

render_pass::render_pass(VkExtent2D extent, render_pass_flags flags,
    VkSampleCountFlagBits samples, VkFormat image_format, VkFormat depth_format) :
    _extent(extent)
{
    std::vector<VkAttachmentDescription> attachments(util::popcount(static_cast<uint32_t>(flags)));
    uint32_t index = 0;
    const bool has_color = static_cast<bool>(flags & render_pass_flags::color);
    const bool has_depth = static_cast<bool>(flags & render_pass_flags::depth);
    const bool has_msaa = static_cast<bool>(flags & render_pass_flags::msaa);
    VkAttachmentReference color_ref{}, depth_ref{}, resolve_ref{};
    VkAttachmentReference* p_color_ref = nullptr, * p_depth_ref = nullptr, * p_resolve_ref = nullptr;
    _samples = has_msaa ? samples : VK_SAMPLE_COUNT_1_BIT;
    _depth_enabled = has_depth;

    if (has_color) {
        attachments[index].format = image_format;
        attachments[index].samples = _samples;
        attachments[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[index].finalLayout = has_msaa ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        color_ref.attachment = index;
        color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_color_ref = &color_ref;
        index += 1;
    }
    if (has_depth) {
        attachments[index].format = depth_format;
        attachments[index].samples = _samples;
        attachments[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[index].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[index].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depth_ref.attachment = index;
        depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        p_depth_ref = &depth_ref;
        index += 1;
    }
    if (has_msaa) {
        attachments[index].format = image_format;
        attachments[index].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[index].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[index].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        resolve_ref.attachment = index;
        resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_resolve_ref = &resolve_ref;
        ++index;
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = has_color;
    subpass.pColorAttachments = p_color_ref;
    subpass.pDepthStencilAttachment = p_depth_ref;
    subpass.pResolveAttachments = p_resolve_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = index;
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    vkCreateRenderPass(device_main::device(), &render_pass_info, NULL_ALLOC, &_pass);

    if (has_depth) {
        _depth_image = image_view_pair(
            _extent, 1, samples, depth_format, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT
        );
    }
    if (has_msaa) {
        _color_image = image_view_pair(
            _extent, 1, samples, image_format, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT
        );
    }
}

render_pass::~render_pass() {
    vkDestroyRenderPass(device_main::device(), _pass, NULL_ALLOC);
}

void render_pass::create_framebuffers(std::span<const image_view> swap_views) {
    _framebuffers.resize(swap_views.size());
    std::array<VkImageView, 3> frame_views{
        _color_image.view().view(), _depth_image.view().view()
    };

    for (auto i = 0u; i < _framebuffers.size(); ++i) {
        frame_views[2] = swap_views[i].view();
        _framebuffers[i] = framebuffer(_pass, frame_views, _extent);
    }
}

void render_pass::start_cmd_pass(const cmd_buffer& buf, uint32_t frame_ind) const {
    buf.begin(0);

    std::vector<VkClearValue> clear_values(1);
    clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (_depth_enabled) {
        clear_values.push_back({ .depthStencil = { 1.0f, 0 } });
    }

    VkRenderPassBeginInfo pass_begin_info{};
    pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_begin_info.renderPass = _pass;
    pass_begin_info.framebuffer = _framebuffers[frame_ind].buffer();
    pass_begin_info.renderArea = { {0, 0} , _extent };
    pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    pass_begin_info.pClearValues = clear_values.data();
    vkCmdBeginRenderPass(buf.buffer(), &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

}