#pragma once

#include "image.hpp"

namespace vkw
{
    class ImageView : public Movable<ImageView>
    {
    public:
        using Movable<ImageView>::operator=;

        ImageView() = default;
        ImageView(ImageView&& oth) = default;
        ImageView(const Device* device, VkImage image, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspectFlags);
        ~ImageView();

        const VkHandle<VkImageView>& imageView() const { return _view; }

    private:
        DevicePtr _device;

        VkHandle<VkImageView> _view;
    };
}