#include "queue_graphics.hpp"

namespace vkw
{
    void GraphicsQueue::transitionImageLayout(const Image& image, VkImageLayout oldLayout, VkImageLayout newLayout) const
    {
        CmdBuffer cmdBuffer(&_pool);
        cmdBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkPipelineStageFlags srcStage, dstStage;

        VkImageMemoryBarrier imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = oldLayout;
        imageBarrier.newLayout = newLayout;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = image.image();
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.levelCount = image.mipLevels();
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.layerCount = 1;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (image.format() == VK_FORMAT_D32_SFLOAT_S8_UINT || image.format() == VK_FORMAT_D24_UNORM_S8_UINT) // if has stencil component
                imageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
        {
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            imageBarrier.srcAccessMask = 0;
            imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imageBarrier.srcAccessMask = 0;
            imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        // else bad, not supported

        vkCmdPipelineBarrier(
            cmdBuffer.buffer(),
            srcStage, dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageBarrier);

        submitCmd(cmdBuffer.buffer());
    }

    void GraphicsQueue::generateMipMaps(const Image& image) const
    {
        CmdBuffer cmdBuffer(&_pool);
        cmdBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image.image();
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = image.extent().width, mipHeight = image.extent().height;
        for (int i = 1; i < image.mipLevels(); ++i)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer.buffer(),
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            vkCmdBlitImage(cmdBuffer.buffer(),
                image.image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image.image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer.buffer(),
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // this transitions to read only layout
        barrier.subresourceRange.baseMipLevel = image.mipLevels() - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer.buffer(),
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        submitCmd(cmdBuffer.buffer());
    }
}
