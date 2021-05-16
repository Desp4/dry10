#include "image.hpp"

#include "vkw/device/g_device.hpp"

namespace dry::vkw {

vk_image::vk_image(
    VkExtent2D dimensions, u32_t mip_lvls, VkSampleCountFlagBits samples, VkFormat img_format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) :
    _extent{ dimensions },
    _mip_levels{ mip_lvls },
    _format{ img_format }
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = _extent.width;
    image_info.extent.height = _extent.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = _mip_levels;
    image_info.arrayLayers = 1;
    image_info.format = _format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = samples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateImage(g_device->handle(), &image_info, null_alloc, &_image);
    VkMemoryRequirements mem_requirements{};
    vkGetImageMemoryRequirements(g_device->handle(), _image, &mem_requirements);

    _memory = vk_device_memory{
        mem_requirements.size,
        g_device->find_memory_type_index(mem_requirements.memoryTypeBits, properties)
    };
    vkBindImageMemory(g_device->handle(), _image, _memory.handle(), 0); // NOTE : no offset
}

vk_image::~vk_image() {
    vkDestroyImage(g_device->handle(), _image, null_alloc);
}

vk_image& vk_image::operator=(vk_image&& oth) {
    // destroy
    vkDestroyImage(g_device->handle(), _image, null_alloc);
    // move
    _image = oth._image;
    _memory = std::move(oth._memory);
    _extent = oth._extent;
    _mip_levels = oth._mip_levels;
    _format = oth._format;
    // null
    oth._image = VK_NULL_HANDLE;
    return *this;
}

}
