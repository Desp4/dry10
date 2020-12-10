#pragma once

#include "imageview.hpp"

namespace vkw
{
    class ImageViewPair : Movable<ImageViewPair>
    {
    public:
        using Movable<ImageViewPair>::operator=;

        ImageViewPair() = default;
        ImageViewPair(ImageViewPair&&) = default;
        ImageViewPair(const Device* device, VkExtent2D dimensions, uint32_t mipLvls, VkSampleCountFlagBits samples, VkFormat imgFormat,
                      VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags);

        const Image& image() const;
        const ImageView& view() const;

    private:
        Image _image;
        ImageView _view;
    };
}