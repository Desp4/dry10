#include "imageviewpair.hpp"

namespace vkw
{
    ImageViewPair::ImageViewPair(const Device* device, VkExtent2D dimensions, uint32_t mipLvls, VkSampleCountFlagBits samples, VkFormat imgFormat,
                                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags) :
        _image(device, dimensions, mipLvls, samples, imgFormat, tiling, usage, properties),
        _view(device, _image.image(), imgFormat, mipLvls, aspectFlags)
    {
    }
}