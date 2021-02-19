#pragma once

#include "queue.hpp"
#include "vkw/image/image.hpp"

namespace dry::vkw {

class queue_graphics : public queue_base {
public:
    using queue_base::queue_base;

    void transition_image_layout(const image_base& image, VkImageLayout layout_old, VkImageLayout layout_new) const;
    void generate_mip_maps(const image_base& image) const;

    // NOTE: msvc doesn't recognize the assignment in base
    queue_graphics& operator=(queue_graphics&& oth) { 
        queue_base::operator=(std::move(oth));
        return *this;
    }
};

}