#include "image.hpp"
#include "vkw/device/device.hpp"

namespace dry::vkw {

image_base::image_base(VkExtent2D dimensions, uint32_t mip_lvls, VkSampleCountFlagBits samples, VkFormat img_format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) :
    _extent(dimensions),
    _mip_levels(mip_lvls),
    _format(img_format)
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

    vkCreateImage(device_main::device(), &image_info, NULL_ALLOC, &_image);
    VkMemoryRequirements mem_requirements{};
    vkGetImageMemoryRequirements(device_main::device(), _image, &mem_requirements);

    _memory = device_memory(
        mem_requirements.size,
        device_main::find_memory_type_index(mem_requirements.memoryTypeBits, properties)
    );
    vkBindImageMemory(device_main::device(), _image, _memory.memory(), 0);
}

image_base::~image_base() {
    vkDestroyImage(device_main::device(), _image, NULL_ALLOC);
}

}