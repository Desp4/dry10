#pragma once

#ifndef DRY_VK_IMAGE_H
#define DRY_VK_IMAGE_H

#include "vkw/device/device.hpp"

namespace dry::vkw {

class vk_image {
public:
    vk_image(const vk_device& device, VkExtent2D dimensions, u32_t mip_lvls, VkSampleCountFlagBits samples,
        VkFormat img_format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memory_usage
    ) noexcept;

    vk_image() noexcept = default;
    vk_image(vk_image&& oth) noexcept { *this = std::move(oth); }
    ~vk_image();

    VkImage handle() const { return _image; }
    VkExtent2D extent() const { return _extent; }
    u32_t mip_levels() const { return _mip_levels; }
    VkFormat format() const { return _format; }

    vk_image& operator=(vk_image&&) noexcept;

private:
    const vk_device* _device = nullptr;
    VkImage _image = VK_NULL_HANDLE;
    VmaAllocation _alloc = VK_NULL_HANDLE;

    VkExtent2D _extent;
    u32_t _mip_levels;
    VkFormat _format;
};

}

#endif