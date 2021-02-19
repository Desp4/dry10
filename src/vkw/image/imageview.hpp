#pragma once

#include "vkw/vkw.hpp"

namespace dry::vkw {

class image_view : public movable<image_view> {
public:
    using movable<image_view>::operator=;

    image_view() = default;
    image_view(image_view&&) = default;
    image_view(VkImage img, VkFormat format, uint32_t mip_lvls, VkImageAspectFlags aspect_flags);
    ~image_view();

    const VkImageView& view() const {
        return _view;
    }

private:
    vk_handle<VkImageView> _view;
};

}