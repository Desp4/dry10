#pragma once

#include "queue.hpp"
#include "../buffer.hpp"
#include "../image/image.hpp"

namespace vkw
{
    class TransferQueue : public Queue
    {
    public:
        using Queue::Queue;

        Buffer createLocalBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data) const;

        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;
        void copyBufferToImage(VkBuffer buffer, const Image& image) const;

        // msvc doesn't recognize the constructor in base
        TransferQueue& operator=(TransferQueue&& oth) { Queue::operator=(std::move(oth)); return *this; }
    };
}