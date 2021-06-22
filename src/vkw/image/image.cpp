#include "image.hpp"

namespace dry::vkw {

vk_image::vk_image(const vk_device& device, VkExtent2D dimensions, u32_t mip_lvls, VkSampleCountFlagBits samples,
    VkFormat img_format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memory_usage) noexcept :
    _device{ &device },
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

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = memory_usage;

    vmaCreateImage(device.allocator(), &image_info, &alloc_info, &_image, &_alloc, nullptr);
}

vk_image::~vk_image() {
    if (_device != nullptr) {
        vmaDestroyImage(_device->allocator(), _image, _alloc);
    }
}

vk_image& vk_image::operator=(vk_image&& oth) noexcept {
    // destroy
    if (_device != nullptr) {
        vmaDestroyImage(_device->allocator(), _image, _alloc);
    }
    // move
    _device = oth._device;
    _image = oth._image;
    _alloc = oth._alloc;
    _extent = oth._extent;
    _mip_levels = oth._mip_levels;
    _format = oth._format;
    // null
    oth._device = nullptr;
    return *this;
}

}
