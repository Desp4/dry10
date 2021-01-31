#pragma once

#include "imageview.hpp"

namespace vkw
{
    class ImageViewPair : public Movable<ImageViewPair>
    {
    public:
        using Movable<ImageViewPair>::operator=;

        ImageViewPair() = default;
        ImageViewPair(ImageViewPair&&) = default;
        ImageViewPair(const Device* device, VkExtent2D dimensions, uint32_t mipLvls, VkSampleCountFlagBits samples, VkFormat imgFormat,
                      VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags);
        ~ImageViewPair() = default;

        const Image& image() const { return _image; }
        const ImageView& view() const { return _view; }

    private:
        Image _image;
        ImageView _view;
    };
}