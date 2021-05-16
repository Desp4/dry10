#include "swapchain_p.hpp"

#include "device/g_device.hpp"

namespace dry::vkw {

vk_swapchain_present::vk_swapchain_present(
    VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform,
    u32_t min_image_count, VkFormat format, VkColorSpaceKHR color_space, VkPresentModeKHR present_mode)
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

    vkCreateSwapchainKHR(g_device->handle(), &swapchain_info, null_alloc, &_swapchain);

    // Create images and their fences
    vkGetSwapchainImagesKHR(g_device->handle(), _swapchain, &_frame_count, nullptr);
    _swap_images.resize(_frame_count);
    _swap_image_views.resize(_frame_count);
    _in_flight_image_fences.resize(_frame_count, VK_NULL_HANDLE);

    vkGetSwapchainImagesKHR(g_device->handle(), _swapchain, &_frame_count, _swap_images.data());
    for (auto i = 0u; i < _frame_count; ++i) {
        _swap_image_views[i] = vk_image_view{ _swap_images[i], format, 1, VK_IMAGE_ASPECT_COLOR_BIT };
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
        vkCreateSemaphore(g_device->handle(), &semaphore_info, null_alloc, _image_available_semaphores.data() + i);
        vkCreateSemaphore(g_device->handle(), &semaphore_info, null_alloc, _render_finished_semaphores.data() + i);
        vkCreateFence(g_device->handle(), &fence_info, null_alloc, _in_flight_fences.data() + i);
    }
}

vk_swapchain_present::~vk_swapchain_present() {
    for (auto i = 0u; i < _frame_count; ++i) {
        vkDestroySemaphore(g_device->handle(), _image_available_semaphores[i], null_alloc);
        vkDestroySemaphore(g_device->handle(), _render_finished_semaphores[i], null_alloc);
        vkDestroyFence(g_device->handle(), _in_flight_fences[i], null_alloc);
    }
    vkDestroySwapchainKHR(g_device->handle(), _swapchain, null_alloc);
}

u32_t vk_swapchain_present::acquire_frame() {
    vkWaitForFences(g_device->handle(), 1, _in_flight_fences.data() + _next_frame, VK_TRUE, UINT64_MAX);

    // _nextFrame and imageIndex are the same after this call
    u32_t image_index = 0;
    vkAcquireNextImageKHR(g_device->handle(), _swapchain, UINT64_MAX, _image_available_semaphores[_next_frame], VK_NULL_HANDLE, &image_index);

    // TODO : this check against null only happens the first few calls when fences are not assigned
    // can't think of a way to remove that check
    if (_in_flight_image_fences[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(g_device->handle(), 1, _in_flight_image_fences.data() + image_index, VK_TRUE, UINT64_MAX);
    }
    _in_flight_image_fences[image_index] = _in_flight_fences[_next_frame];

    _next_frame = (_next_frame + 1) % _frame_count;
    return image_index;
}

void vk_swapchain_present::submit_frame(VkQueue queue, u32_t frame_index, const VkCommandBuffer& cmd_buf) {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    constexpr VkPipelineStageFlags wait_stages[]{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = _image_available_semaphores.data() + frame_index;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf; // NOTE : can take a ref to an r-value on call, it's an array alright

    const VkSemaphore signal_sempahores[]{
        _render_finished_semaphores[frame_index]
    };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_sempahores;
    vkResetFences(g_device->handle(), 1, _in_flight_fences.data() + frame_index);
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

vk_swapchain_present& vk_swapchain_present::operator=(vk_swapchain_present&& oth) {
    // destroy
    for (auto i = 0u; i < _frame_count; ++i) {
        vkDestroySemaphore(g_device->handle(), _image_available_semaphores[i], null_alloc);
        vkDestroySemaphore(g_device->handle(), _render_finished_semaphores[i], null_alloc);
        vkDestroyFence(g_device->handle(), _in_flight_fences[i], null_alloc);
    }
    vkDestroySwapchainKHR(g_device->handle(), _swapchain, null_alloc);
    // move
    _swapchain = oth._swapchain;
    _swap_images = std::move(oth._swap_images);
    _swap_image_views = std::move(oth._swap_image_views);
    _image_available_semaphores = std::move(oth._image_available_semaphores);
    _render_finished_semaphores = std::move(oth._render_finished_semaphores);
    _in_flight_fences = std::move(oth._in_flight_fences);
    _in_flight_image_fences = std::move(oth._in_flight_image_fences);
    _frame_count = oth._frame_count;
    _next_frame = oth._next_frame;
    // null
    oth._swapchain = VK_NULL_HANDLE;
    oth._frame_count = 0;
    return *this;
}

}
