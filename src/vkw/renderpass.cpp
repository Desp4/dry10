#include "renderpass.hpp"

#include <vector>
#include <array>

uint32_t popcount(uint32_t i)
{
    // https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    // TODO : move and come back later replace with intrinsics or fall back to this if none present
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

namespace vkw
{
    RenderPass::RenderPass(const Device* device, VkExtent2D extent, RenderPassFlags flags,
                           VkFormat imageFormat, VkFormat depthFormat, VkSampleCountFlagBits samples) :
        _device(device),
        _extent(extent)
    {
        std::vector<VkAttachmentDescription> attachments(popcount(flags));
        uint32_t index = 0;
        const bool hasColor = flags & RenderPassFlagBits::RenderPass_Color;
        const bool hasDepth = flags & RenderPassFlagBits::RenderPass_Depth;
        const bool hasMSAA = flags & RenderPassFlagBits::RenderPass_MSAA;
        VkAttachmentReference colorRef{}, depthRef{}, resolveRef{};
        VkAttachmentReference* pColorRef = nullptr, * pDepthRef = nullptr, * pResolveRef = nullptr;
        _samples = hasMSAA ? samples : VK_SAMPLE_COUNT_1_BIT;
        _depthEnabled = hasDepth;

        if (hasColor)
        {
            attachments[index].format = imageFormat;
            attachments[index].samples = hasMSAA ? samples: VK_SAMPLE_COUNT_1_BIT;
            attachments[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[index].finalLayout = hasMSAA ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            colorRef.attachment = index;
            colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            pColorRef = &colorRef;
            ++index;
        }
        if (hasDepth)
        {
            attachments[index].format = depthFormat;
            attachments[index].samples = hasMSAA ? samples : VK_SAMPLE_COUNT_1_BIT;
            attachments[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[index].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[index].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            depthRef.attachment = index;
            depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            pDepthRef = &depthRef;
            ++index;
        }
        if (hasMSAA)
        {
            attachments[index].format = imageFormat;
            attachments[index].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[index].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[index].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            resolveRef.attachment = index;
            resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            pResolveRef = &resolveRef;
            ++index;
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = hasColor;
        subpass.pColorAttachments = pColorRef;
        subpass.pDepthStencilAttachment = pDepthRef;
        subpass.pResolveAttachments = pResolveRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = index;
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        vkCreateRenderPass(_device.ptr->device(), &renderPassInfo, NULL_ALLOC, &_pass.handle);

        if (hasDepth)
            _depthImage = ImageViewPair(_device, _extent, 1, samples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        if (hasMSAA)
            _colorImage = ImageViewPair(_device, _extent, 1, samples, imageFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    RenderPass::~RenderPass()
    {
        if (_device) vkDestroyRenderPass(_device.ptr->device(), _pass, NULL_ALLOC);
    }

    void RenderPass::createFrameBuffers(std::span<const ImageView> swapViews)
    {
        _frameBuffers.resize(swapViews.size());
        std::array<VkImageView, 3> frameViews{ _colorImage.view().imageView(), _depthImage.view().imageView() };
        for (int i = 0; i < _frameBuffers.size(); ++i)
        {
            frameViews[2] = swapViews[i].imageView();
            _frameBuffers[i] = FrameBuffer(_device, _pass, frameViews, _extent);
        }
    }

    void RenderPass::startCmdPass(VkCommandBuffer buffer, uint32_t frameInd) const
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(buffer, &beginInfo);

        std::vector<VkClearValue> clearValues(1);
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        if (_depthEnabled)
            clearValues.push_back({ .depthStencil = { 1.0f, 0 } });

        VkRenderPassBeginInfo passBeginInfo{};
        passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        passBeginInfo.renderPass = _pass;
        passBeginInfo.framebuffer = _frameBuffers[frameInd].frameBuffer();
        passBeginInfo.renderArea = { {0, 0} , _extent };
        passBeginInfo.clearValueCount = clearValues.size();
        passBeginInfo.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(buffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    VkRenderPass RenderPass::renderPass() const
    {
        return _pass;
    }

    VkSampleCountFlagBits RenderPass::rasterSampleCount() const
    {
        return _samples;
    }

    VkBool32 RenderPass::depthEnabled() const
    {
        return _depthEnabled;
    }
}