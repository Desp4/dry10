#include "swapchain_p.hpp"
#include "device/device.hpp"

namespace dry::vkw {

swapchain_present::swapchain_present(VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform,
    uint32_t min_image_count, VkFormat format, VkColorSpaceKHR color_space, VkPresentModeKHR present_mode) :
    _next_frame(0)
{
    VkSwapchainCreateInfoKHR swapchain_info{};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = min_image_count;
    swapchain_info.imageFormat = format;
    swapchain_info.imageColorSpace = color_space;
    swapchain_info.imageExtent = extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = transform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    vkCreateSwapchainKHR(device_main::device(), &swapchain_info, NULL_ALLOC, &_swapchain);

    // Create images and their fences
    vkGetSwapchainImagesKHR(device_main::device(), _swapchain, &_frame_count, nullptr);
    _swap_images.resize(_frame_count);
    _swap_image_views.resize(_frame_count);
    _in_flight_image_fences.resize(_frame_count, VK_NULL_HANDLE);

    vkGetSwapchainImagesKHR(device_main::device(), _swapchain, &_frame_count, _swap_images.data());
    for (auto i = 0u; i < _frame_count; ++i) {
        _swap_image_views[i] = image_view(_swap_images[i], format, 1, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    // Create in flight semaphores and fences
    _image_available_semaphores.resize(_frame_count);
    _render_finished_semaphores.resize(_frame_count);
    _in_flight_fences.resize(_frame_count);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto i = 0u; i < _frame_count; ++i) {
        vkCreateSemaphore(device_main::device(), &semaphore_info, NULL_ALLOC, _image_available_semaphores.data() + i);
        vkCreateSemaphore(device_main::device(), &semaphore_info, NULL_ALLOC, _render_finished_semaphores.data() + i);
        vkCreateFence(device_main::device(), &fence_info, NULL_ALLOC, _in_flight_fences.data() + i);
    }
}

swapchain_present::~swapchain_present() {
    for (auto i = 0u; i < _frame_count; ++i) {
        vkDestroySemaphore(device_main::device(), _image_available_semaphores[i], NULL_ALLOC);
        vkDestroySemaphore(device_main::device(), _render_finished_semaphores[i], NULL_ALLOC);
        vkDestroyFence(device_main::device(), _in_flight_fences[i], NULL_ALLOC);
    }
    vkDestroySwapchainKHR(device_main::device(), _swapchain, NULL_ALLOC);
}

uint32_t swapchain_present::acquire_frame() {
    vkWaitForFences(device_main::device(), 1, _in_flight_fences.data() + _next_frame, VK_TRUE, UINT64_MAX);

    // _nextFrame and imageIndex are the same after this call
    uint32_t image_index = 0;
    vkAcquireNextImageKHR(device_main::device(), _swapchain, UINT64_MAX, _image_available_semaphores[_next_frame], VK_NULL_HANDLE, &image_index);

    // TODO : this check against null only happens the first few calls when fences are not assigned
    // can't think of a way to remove that check
    if (_in_flight_image_fences[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(device_main::device(), 1, _in_flight_image_fences.data() + image_index, VK_TRUE, UINT64_MAX);
    }
    _in_flight_image_fences[image_index] = _in_flight_fences[_next_frame];

    _next_frame = (_next_frame + 1) % _frame_count;
    return image_index;
}

void swapchain_present::submit_frame(VkQueue queue, uint32_t frame_index, const VkCommandBuffer* cmd_buf) {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    constexpr VkPipelineStageFlags wait_stages[]{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = _image_available_semaphores.data() + frame_index;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_buf;

    const VkSemaphore signal_sempahores[]{
        _render_finished_semaphores[frame_index]
    };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_sempahores;
    vkResetFences(device_main::device(), 1, _in_flight_fences.data() + frame_index);
    vkQueueSubmit(queue, 1, &submit_info, _in_flight_fences[frame_index]);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_sempahores;

    const VkSwapchainKHR swapChains[]{
        _swapchain
    };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapChains;
    present_info.pImageIndices = &frame_index;
    present_info.pResults = nullptr; // return value success checks, point to vkresult array

    vkQueuePresentKHR(queue, &present_info);
}

}