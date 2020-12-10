#pragma once

#include "../memory.hpp"

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

        VkImage image() const;
        VkExtent2D extent() const;
        uint32_t mipLevels() const;
        VkFormat format() const;

    private:
        DevicePtr _device;

        VkExtent2D _extent;
        uint32_t _mipLevels;
        VkFormat _format;

        VkHandle<VkImage> _image;
        DeviceMemory _memory;
    };
}