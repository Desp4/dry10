#include "swapchain_p.hpp"

namespace vkw
{
    PresentSwapchain::PresentSwapchain(const Device* device, VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform, 
                                       uint32_t minImageCount, VkFormat format, VkColorSpaceKHR colorSpace, VkPresentModeKHR presentMode) :
        _device(device),
        _nextFrame(0)
    {
        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount = minImageCount;
        swapchainInfo.imageFormat = format;
        swapchainInfo.imageColorSpace = colorSpace;
        swapchainInfo.imageExtent = extent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.preTransform = transform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentMode;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

        vkCreateSwapchainKHR(_device->device(), &swapchainInfo, NULL_ALLOC, &_swapchain);

        // Create images and their fences
        vkGetSwapchainImagesKHR(_device->device(), _swapchain, &_frameCount, nullptr);
        _swapImages.resize(_frameCount);
        _swapImageViews.resize(_frameCount);
        _inFlightImageFences.resize(_frameCount, VK_NULL_HANDLE);

        vkGetSwapchainImagesKHR(_device->device(), _swapchain, &_frameCount, _swapImages.data());
        for (int i = 0; i < _frameCount; ++i)
            _swapImageViews[i] = ImageView(_device, _swapImages[i], format, 1, VK_IMAGE_ASPECT_COLOR_BIT);

        // Create in flight semaphores and fences
        _imageAvailableSemaphores.resize(_frameCount);
        _renderFinishedSemaphores.resize(_frameCount);
        _inFlightFences.resize(_frameCount);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < _frameCount; ++i)
        {
            vkCreateSemaphore(_device->device(), &semaphoreInfo, NULL_ALLOC, _imageAvailableSemaphores.data() + i);
            vkCreateSemaphore(_device->device(), &semaphoreInfo, NULL_ALLOC, _renderFinishedSemaphores.data() + i);
            vkCreateFence(_device->device(), &fenceInfo, NULL_ALLOC, _inFlightFences.data() + i);
        }
    }

    PresentSwapchain::~PresentSwapchain()
    {
        if (_device)
        {
            for (int i = 0; i < _frameCount; ++i)
            {
                vkDestroySemaphore(_device->device(), _imageAvailableSemaphores[i], NULL_ALLOC);
                vkDestroySemaphore(_device->device(), _renderFinishedSemaphores[i], NULL_ALLOC);
                vkDestroyFence(_device->device(), _inFlightFences[i], NULL_ALLOC);
            }
            vkDestroySwapchainKHR(_device->device(), _swapchain, NULL_ALLOC);
        }
    }

    uint32_t PresentSwapchain::acquireFrame()
    {
        vkWaitForFences(_device->device(), 1, _inFlightFences.data() + _nextFrame, VK_TRUE, UINT64_MAX);

        // _nextFrame and imageIndex are the same after this call
        uint32_t imageIndex;
        vkAcquireNextImageKHR(_device->device(), _swapchain, UINT64_MAX, _imageAvailableSemaphores[_nextFrame], VK_NULL_HANDLE, &imageIndex);

        // NOTE : this check against null only happens the first few calls when fences are not assigned
        // can't think of a way to remove that check
        if (_inFlightImageFences[imageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(_device->device(), 1, _inFlightImageFences.data() + imageIndex, VK_TRUE, UINT64_MAX);
        _inFlightImageFences[imageIndex] = _inFlightFences[_nextFrame];

        _nextFrame = (_nextFrame + 1) % _frameCount;
        return imageIndex ;
    }

    void PresentSwapchain::submitFrame(VkQueue queue, uint32_t frameIndex, const VkCommandBuffer* cmdBuffer)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkPipelineStageFlags waitStages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = _imageAvailableSemaphores.data() + frameIndex;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = cmdBuffer;

        VkSemaphore signalSempahores[]{ _renderFinishedSemaphores[frameIndex] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSempahores;
        vkResetFences(_device->device(), 1, _inFlightFences.data() + frameIndex);
        vkQueueSubmit(queue, 1, &submitInfo, _inFlightFences[frameIndex]);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSempahores;

        VkSwapchainKHR swapChains[]{ _swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &frameIndex;
        presentInfo.pResults = nullptr; // return value success checks, point to vkresult array

        vkQueuePresentKHR(queue, &presentInfo);
    }
}