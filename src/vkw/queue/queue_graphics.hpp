#pragma once

#ifndef DRY_VK_QUEUE_GRAPHICS_H
#define DRY_VK_QUEUE_GRAPHICS_H

#include "queue.hpp"
#include "vkw/image/image.hpp"

namespace dry::vkw {

class vk_queue_graphics : public vk_queue {
public:
    using vk_queue::vk_queue;

    void transition_image_layout(const vk_image& image, VkImageLayout layout_old, VkImageLayout layout_new) const;
    void generate_mip_maps(const vk_image& image) const;
};

}

#endif
