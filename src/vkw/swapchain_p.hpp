#pragma once

#ifndef DRY_VK_SWAPCHAIN_P_H
#define DRY_VK_SWAPCHAIN_P_H

#include "image/imageview.hpp"

namespace dry::vkw {

// TODO : add recreate option
class vk_swapchain_present {
public:
    vk_swapchain_present(
        VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform,
        u32_t min_image_count, VkFormat format, VkColorSpaceKHR color_space, VkPresentModeKHR present_mode
    );

    vk_swapchain_present() = default;
    vk_swapchain_present(vk_swapchain_present&& oth) { *this = std::move(oth); }
    ~vk_swapchain_present();

    u32_t acquire_frame();
    void submit_frame(VkQueue queue, u32_t frame_index, const VkCommandBuffer& cmd_buf);

    const std::vector<vk_image_view>& swap_views() const { return _swap_image_views; }

    vk_swapchain_present& operator=(vk_swapchain_present&&);

private:
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

    std::vector<VkImage> _swap_images;
    std::vector<vk_image_view> _swap_image_views;
    // NOTE : semaphores and fences are trivial structures but perhaps you'd want to encapsulate later too
    std::vector<VkSemaphore> _image_available_semaphores;
    std::vector<VkSemaphore> _render_finished_semaphores;

    std::vector<VkFence> _in_flight_fences;
    std::vector<VkFence> _in_flight_image_fences;

    // concurrent in flight frames and total frames in a swapchain are the same
    u32_t _frame_count = 0;
    u32_t _next_frame = 0;
};

}

#endif
