#include "queue_transfer.hpp"

namespace vkw
{
    Buffer TransferQueue::createLocalBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data) const
    {
        Buffer stagingBuffer(_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffer.writeToMemory(data, size);

        Buffer ret(_device, size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        copyBuffer(stagingBuffer.buffer(), ret.buffer(), size);
        return ret;
    }

    void TransferQueue::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const
    {
        CmdBuffer cmdBuffer(&_pool);
        cmdBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(cmdBuffer.buffer(), src, dst, 1, &copyRegion);

        submitCmd(cmdBuffer.buffer());
    }

    void TransferQueue::copyBufferToImage(VkBuffer buffer, const Image& image) const
    {
        CmdBuffer cmdBuffer(&_pool);
        cmdBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { image.extent().width, image.extent().height, 1 };

        vkCmdCopyBufferToImage(
            cmdBuffer.buffer(),
            buffer,
            image.image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        submitCmd(cmdBuffer.buffer());
    }
}