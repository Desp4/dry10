#pragma once

#include <vector>

#include "image/imageview.hpp"

namespace dry::vkw {

// TODO : add recreate option
class swapchain_present : public movable<swapchain_present> {
public:
    using movable<swapchain_present>::operator=;

    swapchain_present() = default;
    swapchain_present(swapchain_present&&) = default;
    swapchain_present(VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform,
        uint32_t min_image_count, VkFormat format, VkColorSpaceKHR color_space, VkPresentModeKHR present_mode);
    ~swapchain_present();

    uint32_t acquire_frame();
    void submit_frame(VkQueue queue, uint32_t frame_index, const VkCommandBuffer* cmd_buf);

    const std::vector<image_view>& swap_views() const {
        return _swap_image_views;
    }

private:
    vk_handle<VkSwapchainKHR> _swapchain;

    // concurrent in flight frames and total frames in a swapchain are the same
    util::nullable_base<uint32_t, 0> _frame_count;
    uint32_t _next_frame;

    std::vector<VkImage> _swap_images;
    std::vector<image_view> _swap_image_views;
    // NOTE : semaphores and fences are trivial structures but perhaps you'd want to encapsulate later too
    std::vector<VkSemaphore> _image_available_semaphores;
    std::vector<VkSemaphore> _render_finished_semaphores;

    std::vector<VkFence> _in_flight_fences;
    std::vector<VkFence> _in_flight_image_fences;
};

}