#include "queue_transfer.hpp"
#include <utility>
namespace dry::vkw {

buffer_base queue_transfer::create_local_buffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data) const {
    buffer_base staging_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    staging_buffer.write(data, size);

    buffer_base ret_buf(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    copy_buffer(staging_buffer.buffer(), ret_buf.buffer(), size);
    return ret_buf;
}

void queue_transfer::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const {
    cmd_buffer cmd_buf(&_pool);
    cmd_buf.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(cmd_buf.buffer(), src, dst, 1, &copy_region);

    submit_cmd(cmd_buf.buffer());
}

void queue_transfer::copy_buffer_to_image(VkBuffer buffer, const image_base& image) const {
    cmd_buffer cmd_buf(&_pool);
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

    vkCmdCopyBufferToImage(cmd_buf.buffer(), buffer, image.image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region
    );
    submit_cmd(cmd_buf.buffer());
}

}