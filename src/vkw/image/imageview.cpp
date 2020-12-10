#include "imageview.hpp"

namespace vkw
{
    ImageView::ImageView(const Device* device, VkImage image, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspectFlags) :
        _device(device)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(_device.ptr->device(), &viewInfo, NULL_ALLOC, &_view.handle);
    }

    ImageView::~ImageView()
    {
        if (_device) vkDestroyImageView(_device.ptr->device(), _view, NULL_ALLOC);
    }

    VkImageView ImageView::imageView() const
    {
        return _view;
    }
}