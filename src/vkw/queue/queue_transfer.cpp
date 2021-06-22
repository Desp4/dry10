#include "queue_transfer.hpp"

namespace dry::vkw {

void vk_queue_transfer::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const {
    vk_cmd_buffer cmd_buf{ *_device, _pool };
    cmd_buf.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(cmd_buf.handle(), src, dst, 1, &copy_region);

    submit_cmd(cmd_buf.handle(), true);
}

void vk_queue_transfer::copy_buffer_to_image(VkBuffer buffer, const vk_image& image) const {
    vk_cmd_buffer cmd_buf{ *_device, _pool };
    cmd_buf.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

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

    vkCmdCopyBufferToImage(cmd_buf.handle(),
        buffer, image.handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region
    );
    submit_cmd(cmd_buf.handle());
}

}
