#pragma once

#include "queue.hpp"
#include "../image/image.hpp"

namespace vkw
{
    class GraphicsQueue : public Queue
    {
    public:
        using Queue::Queue;

        void transitionImageLayout(const Image& image, VkImageLayout oldLayout, VkImageLayout newLayout) const;
        void generateMipMaps(const Image& image) const;

        // msvc doesn't recognize the constructor in base
        GraphicsQueue& operator=(GraphicsQueue&& oth) { Queue::operator=(std::move(oth)); return *this; }
    };
}