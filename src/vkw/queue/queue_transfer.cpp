#include "queue_transfer.hpp"

namespace dry::vkw {

vk_buffer vk_queue_transfer::create_local_buffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage) const {
    vk_buffer staging_buffer{ *_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    staging_buffer.write(data, size);

    vk_buffer ret_buf{ *_device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    copy_buffer(staging_buffer.handle(), ret_buf.handle(), size);
    return ret_buf;
}

void vk_queue_transfer::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, bool wait) const {
    vk_cmd_buffer cmd_buf{ *_device, _pool };
    cmd_buf.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(cmd_buf.handle(), src, dst, 1, &copy_region);

    submit_cmd(cmd_buf.handle(), wait);
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
