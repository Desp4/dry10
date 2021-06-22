#include "imageview.hpp"

namespace dry::vkw {

vk_image_view::vk_image_view(const vk_device& device, VkImage image, VkFormat format, u32_t mip_lvls, VkImageAspectFlags aspect_flags) noexcept :
    _device{ &device }
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_lvls;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    vkCreateImageView(_device->handle(), &view_info, null_alloc, &_view);
}

vk_image_view::~vk_image_view() {
    if (_device != nullptr) {
        vkDestroyImageView(_device->handle(), _view, null_alloc);
    }    
}

vk_image_view& vk_image_view::operator=(vk_image_view&& oth) noexcept {
    // destroy
    if (_device != nullptr) {
        vkDestroyImageView(_device->handle(), _view, null_alloc);
    }
    // move
    _device = oth._device;
    _view = oth._view;
    // null
    oth._device = nullptr;
    return *this;
}

}
