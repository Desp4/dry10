#pragma once

#include "vkw/memory.hpp"

namespace vkw
{
    class Image : Movable<Image>
    {
    public:
        using Movable<Image>::operator=;

        Image() = default;
        Image(Image&&) = default;
        Image(const Device* device, VkExtent2D dimensions, uint32_t mipLvls, VkSampleCountFlagBits samples, VkFormat imgFormat,
              VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
        ~Image();

        const VkHandle<VkImage>& image() const { return _image; }
        const VkExtent2D& extent() const { return _extent; }
        const uint32_t& mipLevels() const { return _mipLevels; }
        const VkFormat& format() const { return _format; }

    private:
        DevicePtr _device;

        VkExtent2D _extent;
        uint32_t _mipLevels;
        VkFormat _format;

        VkHandle<VkImage> _image;
        DeviceMemory _memory;
    };
}