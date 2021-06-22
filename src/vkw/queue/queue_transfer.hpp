#pragma once

#ifndef DRY_VK_QUEUE_TRANSFER_H
#define DRY_VK_QUEUE_TRANSFER_H

#include "queue.hpp"
#include "vkw/buffer.hpp"
#include "vkw/image/image.hpp"

namespace dry::vkw {

class vk_queue_transfer : public vk_queue {
public:
    using vk_queue::vk_queue;

    template<typename T>
    vk_buffer create_local_buffer(std::span<const T> values, VkBufferUsageFlags usage) const;

    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;
    void copy_buffer_to_image(VkBuffer buffer, const vk_image& image) const;
};


template<typename T>
vk_buffer vk_queue_transfer::create_local_buffer(std::span<const T> values, VkBufferUsageFlags usage) const {
    vk_buffer staging_buffer{ *_device, values.size_bytes(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
       VMA_MEMORY_USAGE_CPU_ONLY
    };
    staging_buffer.write(values);

    vk_buffer ret_buf{ *_device, values.size_bytes(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VMA_MEMORY_USAGE_GPU_ONLY
    };
    copy_buffer(staging_buffer.handle(), ret_buf.handle(), values.size_bytes());
    return ret_buf;
}

}

#endif
