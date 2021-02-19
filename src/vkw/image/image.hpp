#pragma once

#include "vkw/memory.hpp"

namespace dry::vkw {

class image_base : public movable<image_base> {
public:
    using movable<image_base>::operator=;

    image_base() = default;
    image_base(image_base&&) = default;
    image_base(VkExtent2D dimensions, uint32_t mip_lvls, VkSampleCountFlagBits samples, VkFormat img_format,
          VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    ~image_base();

    const VkImage& image() const {
        return _image;
    }
    const VkExtent2D& extent() const {
        return _extent;
    }
    uint32_t mip_levels() const {
        return _mip_levels;
    }
    VkFormat format() const {
        return _format;
    }

private:
    VkExtent2D _extent;
    uint32_t _mip_levels;
    VkFormat _format;

    vk_handle<VkImage> _image;
    device_memory _memory;
};

}