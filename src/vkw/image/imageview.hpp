#pragma once

#ifndef DRY_VK_IMAGEVIEW_H
#define DRY_VK_IMAGEVIEW_H

#include "vkw/vkw.hpp"

namespace dry::vkw {

class vk_image_view {
public:
    vk_image_view(VkImage image, VkFormat format, u32_t mip_lvls, VkImageAspectFlags aspect_flags);

    vk_image_view() = default;
    vk_image_view(vk_image_view&& oth) { *this = std::move(oth); }
    ~vk_image_view();

    VkImageView handle() const { return _view; }

    vk_image_view& operator=(vk_image_view&&);

private:
    VkImageView _view = VK_NULL_HANDLE;
};

}

#endif
