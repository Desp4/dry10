#pragma once

#include <vector>

#include "image/imageview.hpp"

namespace vkw
{
    // TODO : FUTURE : add recreate option
    struct PresentFrame
    {
        uint32_t frameIndex;
        uint32_t imageIndex;
    };

    class PresentSwapchain : public Movable<PresentSwapchain>
    {
    public:
        using Movable<PresentSwapchain>::operator=;

        PresentSwapchain() = default;
        PresentSwapchain(PresentSwapchain&&) = default;
        PresentSwapchain(const Device* device, VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform,
                         uint32_t imageCount, VkFormat format, VkColorSpaceKHR colorSpace, VkPresentModeKHR presentMode);
        ~PresentSwapchain();

        PresentFrame acquireFrame();
        void submitFrame(VkQueue queue, PresentFrame frame, const VkCommandBuffer* cmdBuffer);

        const std::vector<ImageView>& swapViews() const { return _swapImageViews; }

    private:
        DevicePtr _device;

        VkHandle<VkSwapchainKHR> _swapchain;

        // concurrent in flight frames and total frames in a swapchain are the same
        uint32_t _frameCount;
        uint32_t _currentFrame;

        std::vector<VkImage> _swapImages;
        std::vector<ImageView> _swapImageViews;
        // NOTE : semaphores and fences are trivial structures but perhaps you'd want to encapsulate later too
        std::vector<VkSemaphore> _imageAvailableSemaphores;
        std::vector<VkSemaphore> _renderFinishedSemaphores;

        std::vector<VkFence> _inFlightFences;
        std::vector<VkFence> _inFlightImageFences;
    };
}