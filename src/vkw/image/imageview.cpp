#include "imageview.hpp"
#include "vkw/device/device.hpp"

namespace dry::vkw {

image_view::image_view(VkImage img, VkFormat format, uint32_t mip_lvls, VkImageAspectFlags aspect_flags) {
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = img;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_lvls;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    vkCreateImageView(device_main::device(), &view_info, NULL_ALLOC, &_view);
}

image_view::~image_view() {
    vkDestroyImageView(device_main::device(), _view, NULL_ALLOC);
}

}