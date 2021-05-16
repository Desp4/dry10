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
    vk_buffer create_local_buffer(const T& value, VkBufferUsageFlags usage) const { 
        return create_local_buffer(&value, sizeof(T), usage);
    }
    template<typename T>
    vk_buffer create_local_buffer(const std::vector<T>& values, VkBufferUsageFlags usage) const {
        return create_local_buffer(values.data(), sizeof(T) * values.size(), usage);
    }
    template<typename T>
    vk_buffer create_local_buffer(std::span<const T> values, VkBufferUsageFlags usage) const {
        return create_local_buffer(values.data(), values.size_bytes(), usage);
    }
    vk_buffer create_local_buffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage) const;

    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;
    void copy_buffer_to_image(VkBuffer buffer, const vk_image& image) const;
};

}

#endif
