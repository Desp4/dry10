#pragma once

#include "queue.hpp"
#include "vkw/buffer.hpp"
#include "vkw/image/image.hpp"

namespace dry::vkw {

class queue_transfer : public queue_base {
public:
    using queue_base::queue_base;

    buffer_base create_local_buffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data) const;

    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;
    void copy_buffer_to_image(VkBuffer buffer, const image_base& image) const;

    // NOTE: msvc doesn't recognize the assignment in base
    queue_transfer& operator=(queue_transfer&& oth) {
        queue_base::operator=(std::move(oth));
        return *this;
    }
};

}