#pragma once

#ifndef DRY_VK_IMAGE_H
#define DRY_VK_IMAGE_H

#include "vkw/memory.hpp"

namespace dry::vkw {

class vk_image {
public:
    vk_image(
        VkExtent2D dimensions, u32_t mip_lvls, VkSampleCountFlagBits samples, VkFormat img_format,
        VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties
    );

    vk_image() = default;
    vk_image(vk_image&& oth) { *this = std::move(oth); }
    ~vk_image();

    VkImage handle() const { return _image; }
    VkExtent2D extent() const { return _extent; }
    u32_t mip_levels() const { return _mip_levels; }
    VkFormat format() const { return _format; }

    vk_image& operator=(vk_image&&);

private:
    VkImage _image = VK_NULL_HANDLE;
    vk_device_memory _memory;

    VkExtent2D _extent;
    u32_t _mip_levels;
    VkFormat _format;
};

}

#endif