#include "image.hpp"

namespace vkw
{
    Image::Image(const Device* device, VkExtent2D dimensions, uint32_t mipLvls, VkSampleCountFlagBits samples, VkFormat imgFormat,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) :
        _device(device),
        _extent(dimensions),
        _mipLevels(mipLvls),
        _format(imgFormat)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = _extent.width;
        imageInfo.extent.height = _extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = _mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = _format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(_device.ptr->device(), &imageInfo, NULL_ALLOC, &_image.handle);
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(_device.ptr->device(), _image, &memRequirements);

        _memory = DeviceMemory(device, memRequirements.size,
            DeviceMemory::findMemoryTypeIndex(*_device, memRequirements.memoryTypeBits, properties));

        vkBindImageMemory(_device.ptr->device(), _image, _memory.memory(), 0);
    }

    Image::~Image()
    {
        if (_device) vkDestroyImage(_device.ptr->device(), _image, NULL_ALLOC);
    }

    VkImage Image::image() const
    {
        return _image;
    }

    VkExtent2D Image::extent() const
    {
        return _extent;
    }

    uint32_t Image::mipLevels() const
    {
        return _mipLevels;
    }

    VkFormat Image::format() const
    {
        return _format;
    }
}