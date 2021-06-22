#pragma once

#ifndef DRY_VK_IMAGEVIEW_H
#define DRY_VK_IMAGEVIEW_H

#include "vkw/device/device.hpp"

namespace dry::vkw {

class vk_image_view {
public:
    vk_image_view(const vk_device& device, VkImage image, VkFormat format, u32_t mip_lvls, VkImageAspectFlags aspect_flags) noexcept;

    vk_image_view() noexcept = default;
    vk_image_view(vk_image_view&& oth) noexcept { *this = std::move(oth); }
    ~vk_image_view();

    VkImageView handle() const { return _view; }

    vk_image_view& operator=(vk_image_view&&) noexcept;

private:
    const vk_device* _device = nullptr;
    VkImageView _view = VK_NULL_HANDLE;
};

}

#endif
