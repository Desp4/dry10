#pragma once

#include "image.hpp"
#include "imageview.hpp"

namespace dry::vkw {

class image_view_pair : public movable<image_view_pair> {
public:
    using movable<image_view_pair>::operator=;

    image_view_pair() = default;
    image_view_pair(image_view_pair&&) = default;
    image_view_pair(VkExtent2D dimensions, uint32_t mip_lvls, VkSampleCountFlagBits samples,
        VkFormat img_format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImageAspectFlags aspect_flags) :
        _image(dimensions, mip_lvls, samples, img_format, tiling, usage, properties),
        _view(_image.image(), img_format, mip_lvls, aspect_flags) {}

    const image_base& image() const {
        return _image;
    }
    const image_view& view() const {
        return _view;
    }

private:
    image_base _image;
    image_view _view;
};

}